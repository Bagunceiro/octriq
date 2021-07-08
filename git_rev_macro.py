import subprocess

revision = (
    subprocess.check_output(["git", "describe", "--dirty=+"])
    .strip()
    .decode("utf-8")
)
print("-DGIT_REV='\"%s\"'" % revision)
