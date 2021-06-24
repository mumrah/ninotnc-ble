import datetime
import subprocess

# See https://docs.platformio.org/en/latest/projectconf/section_env_build.html#built-in-variables
revision = (
    subprocess.check_output(["git", "rev-parse", "--short", "HEAD"])
    .strip()
    .decode("utf-8")
)

try:
    subprocess.check_output(["git", "diff-index", "--quiet", "HEAD", "--"])
except subprocess.CalledProcessError as cpe:
    if cpe.returncode == 1:
        revision = revision + "-dirty"
    else:
        raise cpe

now = datetime.datetime.utcnow().replace(microsecond=0).isoformat()

print(f"-DGIT_REV='\"{revision}\"' -DBUILD_DATE='\"{now}Z\"'")