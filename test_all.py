import os
import subprocess
import sys


ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
QUAD_RUNNER = os.path.join(ROOT_DIR, "quad_runner.py")


TEST_CASES = [
    {"path": "tests/test_if.ou", "should_compile": True, "expected_output": "1\n"},
    {
        "path": "tests/test_while.ou",
        "should_compile": True,
        "expected_output": "0\n1\n2\n",
    },
    {"path": "tests/test_switch.ou", "should_compile": True, "expected_output": "2\n"},
    {"path": "tests/test_break.ou", "should_compile": False},
    {"path": "tests/test_parser_errors.ou", "should_compile": False},
    {"path": "tests/test_switch_2.ou", "should_compile": False},
]


def compile_test(cpq_path, case):
    ou_path = os.path.join(ROOT_DIR, case["path"])
    should_compile = case["should_compile"]
    qud_path = os.path.splitext(ou_path)[0] + ".qud"
    if os.path.exists(qud_path):
        os.remove(qud_path)

    result = subprocess.run([cpq_path, ou_path], cwd=ROOT_DIR)
    ok_compile = (result.returncode == 0) == should_compile

    if not should_compile:
        return ok_compile and not os.path.exists(qud_path), None

    if not ok_compile or not os.path.exists(qud_path):
        return False, "missing output .qud"

    expected_output = case.get("expected_output")
    if expected_output is None:
        return True, None

    run_result = subprocess.run(
        [sys.executable, QUAD_RUNNER, qud_path],
        text=True,
        capture_output=True,
        cwd=ROOT_DIR,
    )
    if run_result.returncode != 0:
        return False, "quad_runner failed"

    actual = run_result.stdout.strip()
    expected = expected_output.strip()
    if actual != expected:
        return (
            False,
            f"runtime output mismatch (expected={expected!r}, actual={actual!r})",
        )

    return True, None


def main():
    cpq_path = os.path.join(ROOT_DIR, "cpq")
    assert cpq_path

    failures = []
    for case in TEST_CASES:
        ok, reason = compile_test(cpq_path, case)
        rel = case["path"]
        if ok:
            print(f"[PASS] {rel}")
        else:
            print(f"[FAIL] {rel}: {reason or 'unexpected result'}")
            failures.append(rel)

    print("-" * 25)
    print(f"{len(TEST_CASES) - len(failures)}/{len(TEST_CASES)} passed")
    print("-" * 25)


main()
