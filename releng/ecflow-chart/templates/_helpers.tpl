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

{{/*
The reference of an image, from one entry of `images`: the registry of `global`
when set, the repository, and the digest or the tag. Called with a dict:
  include "ecflow-chart.image" (dict "root" . "image" .Values.images.server)
*/}}
{{- define "ecflow-chart.image" -}}
{{- $repository := .image.repository -}}
{{- if .root.Values.global.imageRegistry -}}
{{- $parts := splitList "/" $repository -}}
{{- if contains "." (first $parts) -}}
{{- $repository = printf "%s/%s" .root.Values.global.imageRegistry (join "/" (rest $parts)) -}}
{{- else -}}
{{- $repository = printf "%s/%s" .root.Values.global.imageRegistry $repository -}}
{{- end -}}
{{- end -}}
{{- if .image.digest -}}
{{- printf "%s@%s" $repository .image.digest -}}
{{- else -}}
{{- printf "%s:%s" $repository (.image.tag | default "latest") -}}
{{- end -}}
{{- end }}

{{/*
The SFTP accounts, space-separated, as the entrypoint of the sidecar reads them: the
users that have SSH keys, each as `name` or `name:uid` when a uid is given.
*/}}
{{- define "ecflow-chart.sftpUsers" -}}
{{- $entries := list -}}
{{- range .Values.users -}}
{{- if .sshAuthorizedKeys -}}
{{- if .uid -}}
{{- $entries = append $entries (printf "%s:%d" .name (int .uid)) -}}
{{- else -}}
{{- $entries = append $entries .name -}}
{{- end -}}
{{- end -}}
{{- end -}}
{{- join " " $entries -}}
{{- end }}

{{/* The name of the Secret that holds the SFTP keys. */}}
{{- define "ecflow-chart.sftpSecretName" -}}
{{- .Values.sftp.existingSecret | default (printf "%s-sftp-keys" (include "ecflow-chart.fullname" .)) -}}
{{- end }}

{{/* The name of the Secret that holds the administrator's files. */}}
{{- define "ecflow-chart.adminSecretName" -}}
{{- .Values.server.admin.existingSecret | default (printf "%s-admin" (include "ecflow-chart.fullname" .)) -}}
{{- end }}

{{/* The name of the TLS Secret of the reverse proxy. */}}
{{- define "ecflow-chart.revproxyTlsSecretName" -}}
{{- .Values.revproxy.tls.existingSecret | default (printf "%s-revproxy-tls" (include "ecflow-chart.fullname" .)) -}}
{{- end }}

{{/* The host published by the Ingress. */}}
{{- define "ecflow-chart.ingressHost" -}}
{{- .Values.ingress.host | default (printf "%s.%s" .Values.ingress.hostPrefix .Values.ingress.domain) -}}
{{- end }}

{{/* The accounts that get a home directory: every user with a uid, as `name:uid`. */}}
{{- define "ecflow-chart.homeUsers" -}}
{{- $entries := list -}}
{{- range .Values.users -}}
{{- if .uid -}}
{{- $entries = append $entries (printf "%s:%d" .name (int .uid)) -}}
{{- end -}}
{{- end -}}
{{- join " " $entries -}}
{{- end }}

{{/* Whether the homes volume is enabled. */}}
{{- define "ecflow-chart.homeEnabled" -}}
{{- $home := .Values.server.persistence.home -}}
{{- not (and (hasKey $home "enabled") (not $home.enabled)) -}}
{{- end }}
