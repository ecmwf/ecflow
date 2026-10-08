{{/*
SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
SPDX-License-Identifier: Apache-2.0
*/}}

{{/*
Named templates shared by the templates of the chart. A named template is called with
`include "<name>" .` and renders to a string; the ones below follow the conventions of
`helm create` (chart name, full name, labels), truncated to the 63 characters that a
Kubernetes name allows.
*/}}

{{/* The chart name, or its override. */}}
{{- define "ecflow-chart.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
The name every object of the release is derived from: `fullnameOverride`, or the
release name, or `<release>-<chart>` when the release name does not already contain
the chart name.
*/}}
{{- define "ecflow-chart.fullname" -}}
{{- if .Values.fullnameOverride }}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- $name := default .Chart.Name .Values.nameOverride }}
{{- if contains $name .Release.Name }}
{{- .Release.Name | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" }}
{{- end }}
{{- end }}
{{- end }}

{{/* The `helm.sh/chart` label: chart name and version. */}}
{{- define "ecflow-chart.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/* The labels common to every object of the release. */}}
{{- define "ecflow-chart.labels" -}}
helm.sh/chart: {{ include "ecflow-chart.chart" . }}
{{ include "ecflow-chart.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/*
The labels that select the Pods of the release. Selectors are immutable once a
Deployment exists, so this set never changes between versions of the chart.
*/}}
{{- define "ecflow-chart.selectorLabels" -}}
app.kubernetes.io/name: {{ include "ecflow-chart.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}
