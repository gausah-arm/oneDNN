#! /bin/bash

# *******************************************************************************
# Copyright 2025 Arm Limited and affiliates.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# *******************************************************************************

# Usage: bash bench_nightly_performance.sh {baseline_benchdnn_executable} {benchdnn_executable} {baseline_results_file} {new_results_file}

IFS=$'\n' # Prevents shuffling from using spaces as delimiters

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

TESTS=(
        "$1 --matmul --mode=P --perf-template=%prb%,%-time%,%-ctime% --batch=${SCRIPT_DIR}/inputs/matmul_nightly >> $3"
        "$2 --matmul --mode=P --perf-template=%prb%,%-time%,%-ctime% --batch=${SCRIPT_DIR}/inputs/matmul_nightly >> $4"
        "$1 --conv --mode=P --perf-template=%prb%,%-time%,%-ctime% --batch=${SCRIPT_DIR}/inputs/conv_nightly >> $3"
        "$2 --conv --mode=P --perf-template=%prb%,%-time%,%-ctime% --batch=${SCRIPT_DIR}/inputs/conv_nightly >> $4"
        "$1 --eltwise --mode=P --perf-template=%prb%,%-time%,%-ctime% --batch=${SCRIPT_DIR}/inputs/eltwise_nightly >> $3"
        "$2 --eltwise --mode=P --perf-template=%prb%,%-time%,%-ctime% --batch=${SCRIPT_DIR}/inputs/eltwise_nightly >> $4"
        "$1 --reorder --mode=P --perf-template=%prb%,%-time%,%-ctime% --batch=${SCRIPT_DIR}/inputs/reorder_nightly >> $3"
        "$2 --reorder --mode=P --perf-template=%prb%,%-time%,%-ctime% --batch=${SCRIPT_DIR}/inputs/reorder_nightly >> $4"
    )

N=5

for i in $( seq $N )
do
    echo "Testing loop ${i} / ${N}..."

    TESTS=( $(shuf -e "${TESTS[@]}") )

    for test in "${TESTS[@]}"
    do
        echo "Starting ${test}"
        SECONDS=0
        eval $test
        duration=$SECONDS
        echo "Completed in $((duration / 60)):$((duration % 60))"
    done
done
