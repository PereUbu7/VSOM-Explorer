#pragma once

#include <libsom/SOM.hpp>
#include <libsom/DataSet.hpp>
#include <imgui/imgui.h>

namespace VSOMExplorer
{
    void RenderExplorer(Som& som, DataSet& dataset);
}