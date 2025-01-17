import sys
import os

def process_lines(file_content, extract_index, insert_index, prefix="", suffix="", offset=0):
    # Split the content into lines
    lines = file_content.splitlines()

    # Start processing lines from the given offset
    processed_lines = []
    for line in lines[offset:]:
        # Split each line by ", "
        values = line.split(", ")
        
        # Check if both the extract_index and insert_index are valid
        if extract_index < len(values):
            # Extract the value, modify it with prefix and suffix
            modified_value = f"{prefix}{values[extract_index]}{suffix}"
            
            # Insert the modified value at the insert_index, shifting the rest to the right
            if insert_index <= len(values):
                values.insert(insert_index, modified_value)
            else:
                values.append(modified_value)  # In case the insert index is beyond the current length
        else:
            print(f"Warning: Skipping line due to invalid index: {line}")
        
        # Join the values back into a single string
        processed_lines.append(", ".join(values))

    return "\n".join(processed_lines)

def main(input_file, extract_index, insert_index, prefix="", suffix="", outtag=None, offset=0):
    # Read the content of the input file
    with open(input_file, 'r') as file:
        content = file.read()

    # Process the lines, modifying the value and inserting it at the specified index
    processed_content = process_lines(content, extract_index, insert_index, prefix, suffix, offset)

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
        file.write(f"# Extracted value from index: {extract_index}\n")
        file.write(f"# Inserted value at index: {insert_index}\n")
        file.write(f"# Prefix: {prefix}\n")
        file.write(f"# Suffix: {suffix}\n")
        file.write(f"# Starting from line {offset}\n\n")
        file.write(processed_content)

    print(f"Processed content stored in {output_file}")

if __name__ == "__main__":
    # Ensure the script is passed the necessary arguments
    if len(sys.argv) < 4:
        print("Usage: python modify_and_insert.py <input_file> <extract_index> <insert_index> [-prefix=<prefix>] [-suffix=<suffix>] [-outtag=<outtag>] [-offset=<offset>]")
        sys.exit(1)

    input_file = sys.argv[1]
    extract_index = int(sys.argv[2])
    insert_index = int(sys.argv[3])

    # Optional parameters
    prefix = ""
    suffix = ""
    outtag = None
    offset = 0  # Default to starting from the first line

    # Handle optional flags for prefix, suffix, outtag, and offset
    for arg in sys.argv[4:]:
        if arg.startswith('-prefix='):
            prefix = arg.split('=')[1]
        elif arg.startswith('-suffix='):
            suffix = arg.split('=')[1]
        elif arg.startswith('-outtag='):
            outtag = arg.split('=')[1]
        elif arg.startswith('-offset='):
            offset = int(arg.split('=')[1])

    # Call the main function to process the file
    main(input_file, extract_index, insert_index, prefix, suffix, outtag, offset)
