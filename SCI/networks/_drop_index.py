import sys
import os

def process_lines(file_content, drop_index, offset=0):
    # Split the content into lines
    lines = file_content.splitlines()

    # Start processing lines from the given offset
    processed_lines = []
    for line in lines[offset:]:
        # Split each line by ", " and drop the element at drop_index
        values = line.split(", ")
        if 0 <= drop_index < len(values):
            del values[drop_index]
        # Join the values back into a single string
        processed_lines.append(", ".join(values))

    return "\n".join(processed_lines)

def main(input_file, drop_index, outtag=None, offset=0):
    # Read the content of the input file
    with open(input_file, 'r') as file:
        content = file.read()

    # Process the lines, dropping the value at the specified index
    processed_content = process_lines(content, drop_index, offset)

    # Generate the output file name by appending the output tag
    base_name, ext = os.path.splitext(input_file)
    output_file = f"{base_name}"
    if outtag:
        output_file += f"_{outtag}"
    else:
        output_file += "_res"
    output_file += ext

    # Write the processed content to the output file
    with open(output_file, 'w') as file:
        file.write(f"# Dropped index: {drop_index}\n")
        file.write(f"# Starting from line {offset}\n\n")
        file.write(processed_content)

    print(f"Processed content stored in {output_file}")

if __name__ == "__main__":
    # Ensure the script is passed an input file and an index to drop
    if len(sys.argv) < 3:
        print("Usage: python drop_index.py <input_file> <drop_index> [-outtag=<outtag>] [-offset=<offset>]")
        sys.exit(1)

    input_file = sys.argv[1]
    drop_index = int(sys.argv[2])

    # Optional parameters
    outtag = None
    offset = 0  # Default to starting from the first line

    # Handle optional flags for outtag and offset
    for arg in sys.argv[3:]:
        if arg.startswith('-outtag='):
            outtag = arg.split('=')[1]
        elif arg.startswith('-offset='):
            offset = int(arg.split('=')[1])

    # Call the main function to process the file
    main(input_file, drop_index, outtag, offset)
