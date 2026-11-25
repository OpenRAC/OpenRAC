# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Run the enhanced CI build's tests and check that each required test ran.

Two CTest calls: first the tests that need no Vulkan (short; timing tests run without the
software renderer's load), then every Vulkan test. Most Vulkan tests share one lock
(cmake/GpuTests.cmake) and run one by one; the lock-free ones fill the other CTest slots.
"""

import argparse
import json
import os
import subprocess
import time
import xml.etree.ElementTree as ET
from pathlib import Path

REQUIRED = {
    "probe_gpu_vulkan", "early_lighting_vulkan", "hero_shadow_vulkan",
    "sun_shadow_vulkan", "gtao_vulkan", "probe_bake", "bake_liquids",
    "object_lights", "object_preview", "object_preview_opengl",
    "object_preview_es", "local_lights", "render_bench_smoke",
    "render_equivalence_prepare_vulkan", "render_equivalence_vulkan",
    "render_equivalence_vulkan_remaining", "render_equivalence_vulkan_shard_2",
    "render_equivalence_vulkan_shard_3", "render_equivalence_support",
    "motion_blur", "fsr3_motion", "fsr3_frame", "fsr3_sdk",
    "water_runtime", "effect_material", "vulkan_uniform",
    "vulkan_present", "vulkan_present_timing", "shadow_scroll_vulkan",
    "render_equivalence_seed_vulkan", "vulkan_prewarm",
}


def inventory(build, *selection):
    data = subprocess.check_output(
        ["ctest", "--test-dir", str(build), "--show-only=json-v1", *selection]
    )
    tests = json.loads(data)["tests"]
    if missing := [test["name"] for test in tests if not test.get("command")]:
        raise SystemExit(f"Missing test executables: {missing}")
    return {
        test["name"]: next(
            (prop["value"] for prop in test.get("properties", []) if prop["name"] == "DEPENDS"),
            [],
        )
        for test in tests
    }


def selection_file(build, group):
    return build / f"ci-{group}-tests.txt"


def write_selection(build, group, names):
    path = selection_file(build, group).resolve()
    path.write_text("".join(f"{name}\n" for name in sorted(names)), encoding="utf-8")
    return path


GROUPS = ("host", "vulkan")


def plan(build):
    dependencies = inventory(build)
    tests = set(dependencies)
    if missing := REQUIRED - tests:
        raise SystemExit(f"Missing required tests: {sorted(missing)}")
    # CTest adds fixture setups to a label listing, so the Vulkan group holds its own setups.
    vulkan = set(inventory(build, "-L", "^gpu$"))
    for group, names in zip(GROUPS, (tests - vulkan, vulkan)):
        if external := {dep for name in names for dep in dependencies[name]} - names:
            raise SystemExit(f"{group} tests depend on another group: {sorted(external)}")
        path = write_selection(build, group, names)
        if not names or set(inventory(build, "--tests-from-file", str(path))) != names:
            raise SystemExit(f"Invalid {group} test partition")
        print(f"{group}: {len(names)} tests")


def execute(build, path, report, jobs, env=None):
    start = time.monotonic()
    result = subprocess.run(
        ["ctest", "--test-dir", str(build), "--tests-from-file", str(path),
         "--parallel", str(jobs), "--no-tests=error", "--output-on-failure",
         "--test-output-size-passed", "1048576", "--output-junit", str(report)],
        check=False, env=env,
    )
    if report.exists():
        cases = list(ET.parse(report).iter("testcase"))
        rows = sorted(cases, key=lambda case: float(case.attrib.get("time", 0)), reverse=True)
        summary = [f"### Enhanced tests: {path.stem}", "",
                   f"Elapsed: {time.monotonic() - start:.1f}s; CTest slots: {jobs}", "",
                   "| Test | Seconds | Result |", "| --- | ---: | --- |"]
        for case in rows:
            status = ("failed" if case.find("failure") is not None else
                      "skipped" if case.find("skipped") is not None else "passed")
            summary.append(f"| {case.attrib['name']} | {float(case.attrib.get('time', 0)):.2f} | {status} |")
        if destination := os.environ.get("GITHUB_STEP_SUMMARY"):
            with open(destination, "a", encoding="utf-8") as output:
                output.write("\n".join(summary) + "\n\n")
    result.check_returncode()
    return list(ET.parse(report).iter("testcase"))


def run(build, group):
    path = selection_file(build, group).resolve()
    expected = set(path.read_text(encoding="utf-8").splitlines())
    if set(inventory(build, "--tests-from-file", str(path))) != expected:
        raise SystemExit(f"Restored {group} test inventory differs from the build")
    report = (build / f"ci-{group}-results.xml").resolve()
    cases = execute(build, path, report, 4)
    if len(cases) != len(expected) or {case.attrib["name"] for case in cases} != expected:
        raise SystemExit(f"Incomplete or duplicate results in the {group} test group")
    for case in cases:
        name = case.attrib["name"]
        if name in REQUIRED and case.find("skipped") is not None:
            raise SystemExit(f"Required test skipped: {name}")
        for line in (case.findtext("system-out") or "").splitlines():
            if "test timing " in line:
                print(f"{name}: {line}")


def timing(build):
    shards = {
        "render_equivalence_vulkan", "render_equivalence_vulkan_remaining",
        "render_equivalence_vulkan_shard_2", "render_equivalence_vulkan_shard_3",
    }
    path = write_selection(build, "timing", shards)
    selected = set(inventory(build, "--tests-from-file", str(path)))
    for jobs in (1, 2, 4):
        cache = (build / f"timing-mesa-{jobs}").resolve()
        cache.mkdir()
        report = (build / f"ci-timing-{jobs}.xml").resolve()
        start = time.monotonic()
        cases = execute(build, path, report, jobs, dict(os.environ, MESA_SHADER_CACHE_DIR=str(cache)))
        print(f"{jobs} slots: {time.monotonic() - start:.1f}s including reference setup", flush=True)
        if {case.attrib["name"] for case in cases} != selected:
            raise SystemExit("Timing run did not execute the same CTest selection")
        for case in cases:
            name = case.attrib["name"]
            if case.find("skipped") is not None:
                raise SystemExit(f"Timing test skipped: {name}")
            if name in shards:
                print(f"{name}: {case.attrib['time']}s")
                for line in (case.findtext("system-out") or "").splitlines():
                    if "test timing " in line:
                        print(f"{name}: {line}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("group", choices=("plan", "timing", *GROUPS))
    args = parser.parse_args()
    if args.group == "plan":
        plan(args.build)
    elif args.group == "timing":
        timing(args.build)
    else:
        run(args.build, args.group)
