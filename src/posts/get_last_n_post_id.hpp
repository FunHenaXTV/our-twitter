#pragma once

#include <string>
#include <vector>
#include <iostream>

#include <userver/components/component_list.hpp>

namespace just_post {

void AppendGetLastNPostId(userver::components::ComponentList& component_list);

}  // namespace just_post