// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace Viewer {
enum ViewMode { TreeViewMode, TableViewMode, NoViewMode };
enum ItemRole {
    InfoRole,
    ManualRole,
    ScriptRole,
    JobRole,
    OutputRole,
    WhyRole,
    TriggersRole,
    TimelineRole,
    VariableRole,
    EditRole,
    MessageRole
};
enum AttributeType {
    NoAttribute,
    LabelAttribute,
    MeterAttribute,
    EventAttribute,
    AvisoAttribute,
    MirrorAttribute,
    RepeatAttribute,
    TimeAttribute,
    DateAttribute,
    TriggerAttribute,
    VarAttribute,
    GenVarAttribute,
    LateAttribute,
    LimitAttribute,
    LimiterAttribute
};

enum Param {
    UnknownParam,
    UnknownState,
    ActiveState,
    AbortedState,
    NoIcon,
    WaitIcon,
    RerunIcon,
    MessageIcon,
    CompleteIcon,
    TimeIcon,
    DateIcon,
    ZombieIcon,
    LateIcon
};
} // namespace Viewer
