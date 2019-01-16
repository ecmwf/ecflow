//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VARIABLEMODELDATAOBSERVER_HPP
#define VARIABLEMODELDATAOBSERVER_HPP

class VariableModelDataHandler;

class VariableModelDataObserver
{
public:
      VariableModelDataObserver() = default;
      virtual void notifyCleared(VariableModelDataHandler*)=0;
      virtual void notifyUpdated(VariableModelDataHandler*)=0;
};

#endif // VARIABLEMODELDATAOBSERVER_HPP
