# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

#
# Tests cmake/GenerateGitCommit.cmake, which records the git commit of the
# ecFlow sources in a generated header.
#
# Expected variables (passed via -D on the command line):
#   SCRIPT         path to cmake/GenerateGitCommit.cmake
#   WORK_DIR       scratch directory, owned by this test
#   GIT_EXECUTABLE path to git; the cases using a repository are skipped when empty
#

foreach(var SCRIPT WORK_DIR)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "TestGenerateGitCommit.cmake: ${var} is not set")
  endif()
endforeach()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

set(header "${WORK_DIR}/ecflow_git_commit.h")

# Runs the script and stores the recorded commit in the variable named by out; the header of a previous run is
# removed first, unless KEEP_HEADER is set
function(generate out source_dir)
  if(NOT KEEP_HEADER)
    file(REMOVE "${header}")
  endif()
  execute_process(
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${source_dir} -DOUTPUT=${header} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} ${ARGN}
            -P ${SCRIPT}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output)
  if(NOT result EQUAL 0)
    message(SEND_ERROR "GenerateGitCommit.cmake failed (${result}): ${output}")
    set(${out} "<failed>" PARENT_SCOPE)
    return()
  endif()
  file(STRINGS "${header}" define REGEX "#define ECFLOW_GIT_COMMIT ")
  string(REGEX REPLACE ".*ECFLOW_GIT_COMMIT \"([^\"]*)\".*" "\\1" commit "${define}")
  set(${out} "${commit}" PARENT_SCOPE)
endfunction()

function(expect name actual expected)
  if(actual STREQUAL expected)
    message(STATUS "PASS ${name}: ${actual}")
  else()
    message(SEND_ERROR "FAIL ${name}: expected '${expected}', found '${actual}'")
  endif()
endfunction()

# Runs git in the test repository, with an identity of its own and no hooks, whatever the git configuration of the
# developer
macro(git_command)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} -c user.name=test -c user.email=test@localhost -c commit.gpgsign=false
            -c core.hooksPath=/dev/null ${ARGN}
    WORKING_DIRECTORY "${WORK_DIR}/repo"
    RESULT_VARIABLE git_result
    OUTPUT_VARIABLE git_output
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  if(NOT git_result EQUAL 0)
    message(FATAL_ERROR "git ${ARGN} failed (${git_result})")
  endif()
endmacro()

# A source tree without git history (e.g. a release tarball) records 'none'
file(MAKE_DIRECTORY "${WORK_DIR}/tarball")
generate(commit "${WORK_DIR}/tarball")
expect("source tree without git" "${commit}" "none")

# A commit given by the packager is recorded, shortened to 12 characters, without asking git
generate(commit "${WORK_DIR}/tarball" -DCOMMIT=0123456789abcdef0123456789abcdef01234567)
expect("commit given by the packager" "${commit}" "0123456789ab")

if(GIT_EXECUTABLE)
  # The git configuration of the developer must not affect the fixture: a global configuration whose hook rejects
  # every commit, and whose template installs that hook in every new repository
  file(WRITE "${WORK_DIR}/hooks/pre-commit" "#!/bin/sh\nexit 1\n")
  execute_process(COMMAND chmod +x "${WORK_DIR}/hooks/pre-commit")
  file(COPY "${WORK_DIR}/hooks" DESTINATION "${WORK_DIR}/template")
  file(WRITE "${WORK_DIR}/gitconfig" "[core]\n\thooksPath = ${WORK_DIR}/hooks\n[init]\n\ttemplateDir = ${WORK_DIR}/template\n")
  set(ENV{GIT_CONFIG_GLOBAL} "${WORK_DIR}/gitconfig")

  file(MAKE_DIRECTORY "${WORK_DIR}/repo")
  file(WRITE "${WORK_DIR}/repo/tracked.txt" "first\n")
  git_command(init --quiet --template=)
  git_command(add tracked.txt)
  git_command(commit --quiet -m "First commit")
  git_command(rev-parse HEAD)
  string(SUBSTRING "${git_output}" 0 12 head)

  # A clean repository records the first 12 characters of HEAD
  generate(commit "${WORK_DIR}/repo")
  expect("clean repository" "${commit}" "${head}")

  # An unchanged commit leaves the existing header untouched, so that nothing is recompiled
  execute_process(COMMAND touch -t 200001010000 "${header}")
  set(KEEP_HEADER TRUE)
  generate(commit "${WORK_DIR}/repo")
  unset(KEEP_HEADER)
  file(TIMESTAMP "${header}" modified "%Y%m%d")
  expect("unchanged commit, header left untouched" "${modified}" "20000101")

  # A worktree, where .git is a file (as in a submodule), records the commit it has checked out
  git_command(worktree add --quiet --detach "${WORK_DIR}/worktree")
  if(IS_DIRECTORY "${WORK_DIR}/worktree/.git")
    message(SEND_ERROR "FAIL worktree: expected .git to be a file")
  endif()
  generate(commit "${WORK_DIR}/worktree")
  expect("worktree, where .git is a file" "${commit}" "${head}")

  # Untracked files do not make the tree dirty
  file(WRITE "${WORK_DIR}/repo/untracked.txt" "untracked\n")
  generate(commit "${WORK_DIR}/repo")
  expect("repository with untracked files" "${commit}" "${head}")

  # Uncommitted changes to tracked files add -dirty, and rewrite the existing header
  file(WRITE "${WORK_DIR}/repo/tracked.txt" "changed\n")
  set(KEEP_HEADER TRUE)
  generate(commit "${WORK_DIR}/repo")
  unset(KEEP_HEADER)
  expect("repository with uncommitted changes" "${commit}" "${head}-dirty")

  # A commit given by the packager takes precedence over the repository
  generate(commit "${WORK_DIR}/repo" -DCOMMIT=0123456789abcdef0123456789abcdef01234567)
  expect("commit given by the packager, in a repository" "${commit}" "0123456789ab")
else()
  message(STATUS "SKIP repository cases: git is not available")
endif()

file(REMOVE_RECURSE "${WORK_DIR}")
