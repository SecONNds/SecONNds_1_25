import re
import sys
import os

def extract_pattern(file_content, pattern, flatten=False, prefix="", suffix="", offset=0, dropf=0, dropb=0):
    # Split the content into lines
    lines = file_content.splitlines()

    # Start from the offset line
    lines_to_search = lines[offset:]

    # Join the lines back into a single string for searching
    content_to_search = "\n".join(lines_to_search)

    # Find all matches of the user-defined pattern
    matches = re.findall(pattern, content_to_search, re.DOTALL)
    
    if flatten:
        # Flatten each match into one line by replacing newlines with spaces
        matches = [' '.join(match.split()) for match in matches]
    
    # Drop the first and last characters from each match
    if dropf > 0:
        matches = [match[dropf:] for match in matches]
    if dropb > 0:
        matches = [match[:-dropb] for match in matches]
        
    # Apply the prefix and suffix to each match
    matches = [f"{prefix}{match}{suffix}" for match in matches]
    
    # Join the matched sections with newlines between them
    return "\n".join(matches)

def main(input_file, pattern, flatten, prefix, suffix, outtag, offset, dropf, dropb):
    # Read the content of the input file
    with open(input_file, 'r') as file:
        content = file.read()
    
    # Extract the relevant lines, starting from the offset
    extracted_content = extract_pattern(content, pattern, flatten, prefix, suffix, offset, dropf, dropb)
    
    # Generate the output file name by appending "_res" to the input filename (without extension)
    base_name, ext = os.path.splitext(input_file)
    output_file = f"{base_name}"
    if outtag:
        output_file += f"_{outtag}"
    else:
        output_file += "_res"
    output_file += ext
    
    # Write the pattern as a header and the extracted content to the output file
    with open(output_file, 'w') as file:
        file.write(f"# Extracted with pattern: {pattern}")
        if flatten:
            file.write(" (flattened)")
        file.write("\n")
        file.write(f"# Prefix: {prefix}\n")
        file.write(f"# Suffix: {suffix}\n")
        file.write(f"# Starting from line {offset}\n")
        file.write("\n")
        file.write(extracted_content)

    print(f"Selection stored in {output_file}")

if __name__ == "__main__":
    # Ensure the script is passed an input file and a pattern
    if len(sys.argv) < 3:
        print("Usage: python extract_pattern.py <input_file> <pattern> [--flatten] [-dropf=<dropf>] [-dropb=<dropb>] [-prefix=<prefix>] [-suffix=<suffix>] [-outtag=<outtag>] [-offset=<offset>]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    pattern = sys.argv[2]
    
    # Check for optional flags
    flatten = '--flatten' in sys.argv
    prefix = ""
    suffix = ""
    outtag = ""
    offset = 0  # Default offset is 0 (start from the beginning)
    dropf = 0
    dropb = 0

    # Handle prefix, suffix, outtag, and offset flags
    for arg in sys.argv:
        if arg.startswith('-prefix='):
            prefix = arg.split('=')[1]
        elif arg.startswith('-suffix='):
            suffix = arg.split('=')[1]
        elif arg.startswith('-outtag='):
            outtag = arg.split('=')[1]
        elif arg.startswith('-offset='):
            offset = int(arg.split('=')[1])
        elif arg.startswith('-dropf='):
            dropf = int(arg.split('=')[1])
        elif arg.startswith('-dropb='):
            dropb = int(arg.split('=')[1])

    # Call the main function to process the file
    main(input_file, pattern, flatten, prefix, suffix, outtag, offset, dropf, dropb)
