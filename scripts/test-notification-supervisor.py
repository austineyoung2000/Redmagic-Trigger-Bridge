#!/usr/bin/env python3
"""Exercise recovery ordering and failure gates without Android or hardware writes."""
import pathlib
import subprocess
import tempfile
import unittest

SCRIPT = pathlib.Path(__file__).resolve().parents[1] / 'module/notification-supervisor.sh'
FUNCTIONS = SCRIPT.read_text().split('device=$(getprop ro.product.device)')[0]

class RecoveryTests(unittest.TestCase):
    def run_case(self, body):
        with tempfile.TemporaryDirectory() as directory:
            script = FUNCTIONS + f'\nPATH=/usr/bin:/bin\nLOG={directory}/log\n' + body
            result = subprocess.run(['sh', '-c', script], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_beacon_requires_current_boot_owner_and_desired_state(self):
        self.run_case('''
CURRENT_BOOT=boot-current
BEACON=$(mktemp)
trap 'rm -f "$BEACON"' EXIT
stat() { printf '10500'; }
printf 'version=1\\nboot=boot-old\\npid=12\\nuid=10500\\nstart=99\\ndesired=true\\nconnected=true\\n' > "$BEACON"
read_beacon && exit 91
sed -i 's/boot-old/boot-current/' "$BEACON"
read_beacon || exit 92
sed -i 's/uid=10500/uid=10501/' "$BEACON"
read_beacon && exit 93
sed -i 's/uid=10501/uid=10500/; s/desired=true/desired=false/' "$BEACON"
read_beacon && exit 94
exit 0
''')

    def test_no_access_never_resets_binding(self):
        self.run_case('''
enabled() { return 0; }; approved() { return 1; }
binder_call() { exit 91; }; timeout() { exit 92; }
recover && exit 93
exit 0
''')

    def test_healthy_recovery_does_not_reset(self):
        self.run_case('''
enabled() { return 0; }; approved() { return 0; }
timeout() { return 0; }; sleep() { :; }; healthy() { return 0; }
binder_call() { exit 91; }
recover
''')

    def test_unbind_then_bind_and_callback_required(self):
        self.run_case('''
enabled() { return 0; }; approved() { return 0; }
timeout() { return 0; }; sleep() { :; }; healthy() { return 1; }
read_beacon() { uid=10500; return 0; }
calls=""
binder_call() { calls="$calls $1"; return 0; }
recover && exit 91
[ "$calls" = " 83 81" ] || exit 92
''')

    def test_failed_unbind_still_attempts_rebind_without_claiming_success(self):
        self.run_case('''
enabled() { return 0; }; approved() { return 0; }
timeout() { return 0; }; sleep() { :; }; healthy() { return 1; }
read_beacon() { uid=10500; return 0; }
calls=""
binder_call() { calls="$calls $1"; return 1; }
recover && exit 91
[ "$calls" = " 83 81" ] || exit 92
''')

    def test_access_rechecked_before_reset(self):
        self.run_case('''
enabled() { return 0; }; approvals=0
approved() { approvals=$((approvals + 1)); [ "$approvals" -eq 1 ]; }
timeout() { return 0; }; sleep() { :; }; healthy() { return 1; }
read_beacon() { uid=10500; return 0; }
binder_call() { exit 91; }
recover && exit 92
exit 0
''')

    def test_binder_exception_is_failure_even_with_zero_exit(self):
        self.run_case('''
uid=10500
timeout() { printf 'Result: Parcel(ffffffff 00000000)'; return 0; }
binder_call 83 && exit 91
timeout() { printf 'Result: Parcel(00000000)'; return 0; }
binder_call 83
''')

    def test_variable_spacing_and_multiline_success_receipts(self):
        self.run_case('''
uid=10500
for reply in "Result: Parcel(00000000 '....')" "Result: Parcel(    00000000    '....')" "Result:
 Parcel(
  0x00000000: 00000000 '....')"; do
    timeout() { printf '%s' "$reply"; return 0; }
    binder_call 83 || exit 91
done
reply="Result: Parcel(    fffffffc 00000000)"
binder_call 83 && exit 92
exit 0
''')

if __name__ == '__main__':
    unittest.main()
