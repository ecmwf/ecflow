//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_HPP_
#define VIEWER_HPP_

namespace Viewer
{
    enum ViewMode {TreeViewMode,TableViewMode,NoViewMode};
    enum ItemRole {InfoRole,ManualRole,ScriptRole,JobRole,OutputRole,WhyRole,TriggersRole,TimelineRole,VariableRole,EditRole,MessageRole};
}

#endif
