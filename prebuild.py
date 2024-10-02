import subprocess
import os

root_dir = os.getcwd()
submodule_dir = os.path.join("lib", "esp-hosted")

def run_git_submodule_update(directory):
    try:
        print(f"Updating git submodules in {directory}...")
        subprocess.check_call(["git", "submodule", "update", "--init", "--recursive"], cwd=directory)
        print(f"Git submodules updated successfully in {directory}.")
    except subprocess.CalledProcessError as e:
        print(f"Error updating git submodules in {directory}: {e}")
        raise

def before_build(source, target, env):
    try:
        run_git_submodule_update(root_dir)
        run_git_submodule_update(submodule_dir)

    except Exception as e:
        print(f"Failed to update git submodules: {e}")
        env.Exit(1)

