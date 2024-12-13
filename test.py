import os
import subprocess
import shutil
from concurrent.futures import ThreadPoolExecutor, as_completed

def process_json(file_path):
    """Run the external program on a JSON file."""
    try:
        # Run the external program ./json with the file as an argument
        result = subprocess.run(["./json", file_path], check=True, text=True, capture_output=True)
        return True  # Indicate success
    except subprocess.CalledProcessError:
        return False  # Indicate failure

def clear_directory(directory):
    """Clear all contents of a directory."""
    if os.path.exists(directory):
        shutil.rmtree(directory)  # Delete the directory and its contents
    os.makedirs(directory)  # Recreate the empty directory

def copy_files_parallel(files, source_dir, target_dir):
    """Copy files from source_dir to target_dir using multiple threads."""
    os.makedirs(target_dir, exist_ok=True)  # Ensure the target directory exists

    def copy_file(file):
        src = os.path.join(source_dir, file)
        dest = os.path.join(target_dir, file)
        shutil.copy(src, dest)
        print(f"Copied {file} to {target_dir}")

    with ThreadPoolExecutor(max_workers=16) as executor:  # Adjust thread count based on system
        executor.map(copy_file, files)

def process_file(file_path, filename):
    """Determine the result for a single file based on its prefix."""
    if filename.startswith('y'):
        print(f"Passing: {filename}")
        if process_json(file_path):
            return ("passed_correctly", filename)
        else:
            return ("failed_incorrectly", filename)
    elif filename.startswith('n'):
        print(f"Failing: {filename}")
        if not process_json(file_path):
            return ("failed_correctly", filename)
        else:
            return ("passed_incorrectly", filename)
    elif filename.startswith('i'):
        print(f"Ignoring: {filename}")
        return ("ignored", filename)
    else:
        print(f"Unknown prefix: {filename}")
        return ("unknown", filename)

def main(directory):
    """Run a program on each JSON file and track results using threading."""
    # Directories for incorrectly processed files
    passed_incorrectly_dir = os.path.join(directory, "passed_incorrectly")
    failed_incorrectly_dir = os.path.join(directory, "failed_incorrectly")

    # Clear the target directories at the start
    clear_directory(passed_incorrectly_dir)
    clear_directory(failed_incorrectly_dir)

    # Counters and trackers
    results = {
        "passed_correctly": [],
        "failed_correctly": [],
        "passed_incorrectly": [],
        "failed_incorrectly": []
    }

    # Collect JSON files
    json_files = [
        filename for filename in os.listdir(directory) if filename.endswith('.json')
    ]

    # Process files using ThreadPoolExecutor
    with ThreadPoolExecutor(max_workers=54) as executor:  # Set max_workers to match thread count
        futures = {
            executor.submit(process_file, os.path.join(directory, filename), filename): filename
            for filename in json_files
        }

        for future in as_completed(futures):
            category, filename = future.result()
            if category in results:
                results[category].append(filename)

    # Copy files to respective directories
    copy_files_parallel(results["passed_incorrectly"], directory, passed_incorrectly_dir)
    copy_files_parallel(results["failed_incorrectly"], directory, failed_incorrectly_dir)

    # Summary of results
    print("\nSummary:")
    print(f"Passed Correctly: {len(results['passed_correctly'])}")
    print(f"Failed Correctly: {len(results['failed_correctly'])}")
    print(f"Passed Incorrectly: {len(results['passed_incorrectly'])}")
    print(f"Failed Incorrectly: {len(results['failed_incorrectly'])}")
    print(f"\nCopied incorrect files to:")
    print(f" - {passed_incorrectly_dir}")
    print(f" - {failed_incorrectly_dir}")

if __name__ == "__main__":
    # Replace 'test_parsing' with your actual directory path
    directory = "test_parsing"
    main(directory)
