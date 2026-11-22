#include "render_graph.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <functional>
#include <new>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nle {
namespace {

[[nodiscard]] bool contains_edge(
    const std::vector<std::size_t>& edges,
    const std::size_t target
) {
    return std::find(edges.begin(), edges.end(), target) != edges.end();
}

void add_edge(
    std::vector<std::vector<std::size_t>>& edges,
    std::vector<std::size_t>& indegree,
    const std::size_t prerequisite,
    const std::size_t dependent
) {
    if (prerequisite == dependent || contains_edge(edges[prerequisite], dependent)) {
        return;
    }

    edges[prerequisite].push_back(dependent);
    ++indegree[dependent];
}

} // namespace

ErrorCode RenderGraph::create_resource(
    const std::string_view name,
    const bool imported,
    RenderGraphResourceId& result
) {
    result = InvalidResource;
    if (name.empty()) {
        return ErrorCode::InvalidParameter;
    }
    if (resources_.size() >= std::numeric_limits<RenderGraphResourceId>::max()) {
        return ErrorCode::OutOfMemory;
    }

    try {
        resources_.push_back(Resource{std::string(name), imported});
    } catch (const std::bad_alloc&) {
        return ErrorCode::OutOfMemory;
    } catch (const std::length_error&) {
        return ErrorCode::OutOfMemory;
    }

    result = static_cast<RenderGraphResourceId>(resources_.size());
    return ErrorCode::Ok;
}

ErrorCode RenderGraph::add_pass(
    const std::string_view name,
    const std::span<const RenderGraphResourceUse> resources,
    RenderGraphPassId& result
) {
    result = InvalidPass;
    if (name.empty()) {
        return ErrorCode::InvalidParameter;
    }
    if (passes_.size() >= std::numeric_limits<RenderGraphPassId>::max()) {
        return ErrorCode::OutOfMemory;
    }

    for (std::size_t index = 0; index < resources.size(); ++index) {
        const RenderGraphResourceUse& use = resources[index];
        if (use.resource == InvalidResource || use.resource > resources_.size()) {
            return ErrorCode::InvalidParameter;
        }

        switch (use.access) {
            case RenderGraphAccess::Read:
            case RenderGraphAccess::Write:
            case RenderGraphAccess::ReadWrite:
                break;
            default:
                return ErrorCode::InvalidParameter;
        }

        if (std::find_if(
                resources.begin(),
                resources.begin() + static_cast<std::ptrdiff_t>(index),
                [&use](const RenderGraphResourceUse& previous) {
                    return previous.resource == use.resource;
                }) != resources.begin() + static_cast<std::ptrdiff_t>(index)) {
            return ErrorCode::InvalidParameter;
        }
    }

    try {
        passes_.push_back(Pass{
            std::string(name),
            std::vector<RenderGraphResourceUse>(resources.begin(), resources.end()),
            {}
        });
    } catch (const std::bad_alloc&) {
        return ErrorCode::OutOfMemory;
    } catch (const std::length_error&) {
        return ErrorCode::OutOfMemory;
    }

    result = static_cast<RenderGraphPassId>(passes_.size());
    return ErrorCode::Ok;
}

ErrorCode RenderGraph::add_dependency(
    const RenderGraphPassId dependent,
    const RenderGraphPassId prerequisite
) {
    if (dependent == InvalidPass || prerequisite == InvalidPass ||
        dependent > passes_.size() || prerequisite > passes_.size() ||
        dependent == prerequisite) {
        return ErrorCode::InvalidParameter;
    }

    Pass& pass = passes_[dependent - 1];
    if (std::find(pass.dependencies.begin(), pass.dependencies.end(), prerequisite) ==
        pass.dependencies.end()) {
        try {
            pass.dependencies.push_back(prerequisite);
        } catch (const std::bad_alloc&) {
            return ErrorCode::OutOfMemory;
        } catch (const std::length_error&) {
            return ErrorCode::OutOfMemory;
        }
    }
    return ErrorCode::Ok;
}

ErrorCode RenderGraph::compile(std::vector<RenderGraphPassId>& order) const {
    order.clear();

    try {
    const std::size_t pass_count = passes_.size();
    std::vector<std::vector<std::size_t>> edges(pass_count);
    std::vector<std::size_t> indegree(pass_count, 0);
    std::vector<std::ptrdiff_t> last_writer(resources_.size(), -1);
    std::vector<std::vector<std::size_t>> readers(resources_.size());

    for (std::size_t pass_index = 0; pass_index < pass_count; ++pass_index) {
        const Pass& pass = passes_[pass_index];

        for (const RenderGraphPassId prerequisite : pass.dependencies) {
            add_edge(edges, indegree, prerequisite - 1, pass_index);
        }

        for (const RenderGraphResourceUse& use : pass.resources) {
            const std::size_t resource_index = use.resource - 1;
            const std::ptrdiff_t writer = last_writer[resource_index];
            const bool imported = resources_[resource_index].imported;
            const bool reads = use.access != RenderGraphAccess::Write;
            const bool writes = use.access != RenderGraphAccess::Read;

            if (reads && writer < 0 && !imported) {
                return ErrorCode::InvalidState;
            }

            if (writer >= 0) {
                add_edge(
                    edges,
                    indegree,
                    static_cast<std::size_t>(writer),
                    pass_index
                );
            }

            if (writes) {
                for (const std::size_t reader : readers[resource_index]) {
                    add_edge(edges, indegree, reader, pass_index);
                }
                readers[resource_index].clear();
                last_writer[resource_index] = static_cast<std::ptrdiff_t>(pass_index);
            } else {
                readers[resource_index].push_back(pass_index);
            }
        }
    }

    std::priority_queue<
        std::size_t,
        std::vector<std::size_t>,
        std::greater<>
    > ready;
    for (std::size_t index = 0; index < pass_count; ++index) {
        if (indegree[index] == 0) {
            ready.push(index);
        }
    }

    std::vector<RenderGraphPassId> compiled;
    compiled.reserve(pass_count);

    while (!ready.empty()) {
        const std::size_t next = ready.top();
        ready.pop();
        compiled.push_back(static_cast<RenderGraphPassId>(next + 1));

        for (const std::size_t dependent : edges[next]) {
            --indegree[dependent];
            if (indegree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    if (compiled.size() != pass_count) {
        return ErrorCode::InvalidState;
    }

    order = std::move(compiled);
    return ErrorCode::Ok;
    } catch (const std::bad_alloc&) {
        order.clear();
        return ErrorCode::OutOfMemory;
    } catch (const std::length_error&) {
        order.clear();
        return ErrorCode::OutOfMemory;
    }
}

std::size_t RenderGraph::get_resource_count() const noexcept {
    return resources_.size();
}

std::size_t RenderGraph::get_pass_count() const noexcept {
    return passes_.size();
}

void RenderGraph::clear() noexcept {
    resources_.clear();
    passes_.clear();
}

} // namespace nle
