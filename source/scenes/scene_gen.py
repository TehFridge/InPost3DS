import os
import sys
import re

def to_camel_case(snake_str):
    components = snake_str.split('_')
    return "".join(x.title() for x in components)

def to_upper_snake(name):
    return name.upper().replace(" ", "_")

def get_next_scene_id(content):
    matches = re.findall(r'=\s*(\d+)', content)
    if not matches:
        return 10 
    numbers = [int(m) for m in matches]
    return max(numbers) + 1

def generate_files(scene_name):
    base_name = scene_name.lower()
    class_name = to_camel_case(base_name)
    upper_name = to_upper_snake(base_name)
    
    header_file = f"scene_{base_name}.h"
    source_file = f"scene_{base_name}.c"
    
    # --- Generate Header ---
    header_content = f"""#ifndef SCENE_{upper_name}_H
#define SCENE_{upper_name}_H
#include <stdint.h>

void scene{class_name}Init(void);
void scene{class_name}Update(uint32_t kDown, uint32_t kHeld);
void scene{class_name}Render(void);
void scene{class_name}Exit(void);

#endif
"""
    with open(header_file, 'w') as f:
        f.write(header_content)
    print(f"Created {header_file}")

    # --- Generate Source ---
    source_content = f"""#include <3ds.h>
#include <citro2d.h>
#include "{header_file}"
#include "scene_manager.h"
#include "../main.h"
#include "../utils/sprites.h"
#include "../utils/button.h"
#include "../utils/cwav_shit.h"

void scene{class_name}Init(void) {{
    
}}

void scene{class_name}Update(uint32_t kDown, uint32_t kHeld) {{
    
}}

void scene{class_name}Render(void) {{
    
}}

void scene{class_name}Exit(void) {{
    
}}
"""
    with open(source_file, 'w') as f:
        f.write(source_content)
    print(f"Created {source_file}")

def update_scene_types(scene_name):
    file_path = "scene_types.h"
    if not os.path.exists(file_path):
        print(f"Error: {file_path} not found.")
        return

    with open(file_path, 'r') as f:
        lines = f.readlines()

    upper_name = to_upper_snake(scene_name)
    enum_name = f"SCENE_{upper_name}"

    # Check if already exists
    for line in lines:
        if enum_name in line:
            print(f"Enum {enum_name} already exists. Skipping.")
            return

    # 1. Find the closing brace of the enum
    close_idx = -1
    for i, line in enumerate(lines):
        if "SceneType;" in line and "}" in line:
            close_idx = i
            break
    
    if close_idx == -1:
        print("Error: Could not find closing '} SceneType;' in scene_types.h")
        return

    # 2. Find the next available ID
    full_content = "".join(lines)
    next_id = get_next_scene_id(full_content)

    # 3. Fix the PREVIOUS line to ensure it has a comma
    # Scan backwards from close_idx to find the last real code line
    prev_data_idx = -1
    for i in range(close_idx - 1, -1, -1):
        stripped = lines[i].strip()
        if stripped and not stripped.startswith("//") and not stripped.startswith("/*"):
            prev_data_idx = i
            break
    
    if prev_data_idx != -1:
        line = lines[prev_data_idx].rstrip()
        # Handle inline comments like "SCENE_X = 1 // comment"
        comment_idx = line.find("//")
        
        if comment_idx != -1:
            code_part = line[:comment_idx].rstrip()
            comment_part = line[comment_idx:]
            if not code_part.endswith(",") and not code_part.endswith("{"):
                lines[prev_data_idx] = code_part + "," + " " + comment_part + "\n"
        else:
            if not line.endswith(",") and not line.endswith("{"):
                lines[prev_data_idx] = line + ",\n" 
            else:
                # If it already has a comma, just ensure newline
                lines[prev_data_idx] = line + "\n"

    # 4. Insert the NEW line
    lines.insert(close_idx, f"    {enum_name} = {next_id}\n")

    with open(file_path, 'w') as f:
        f.writelines(lines)
    print(f"Added {enum_name} to {file_path}")

def update_scene_manager(scene_name):
    file_path = "scene_manager.c"
    if not os.path.exists(file_path):
        print(f"Error: {file_path} not found.")
        return

    with open(file_path, 'r') as f:
        lines = f.readlines()

    base_name = scene_name.lower()
    class_name = to_camel_case(base_name)
    upper_name = to_upper_snake(base_name)
    header_include = f'#include "scene_{base_name}.h"\n'

    # 1. Add Include if missing
    has_include = False
    for line in lines:
        if header_include.strip() in line:
            has_include = True
            break
            
    if not has_include:
        last_include_idx = 0
        for i, line in enumerate(lines):
            if line.strip().startswith("#include"):
                last_include_idx = i
        lines.insert(last_include_idx + 1, header_include)
        print(f"Added include to {file_path}")

    # Helper to insert case statements safely
    def insert_case(target_func, target_switch, new_case):
        in_func = False
        in_switch = False
        
        for i in range(len(lines)):
            if target_func in lines[i]:
                in_func = True
            
            if in_func and target_switch in lines[i]:
                in_switch = True
            
            # Insert before 'default:' or closing brace '}'
            if in_func and in_switch:
                if "default:" in lines[i] or "}" in lines[i]:
                    # Check for duplicates
                    exists = False
                    # Scan a few lines back to see if we just added it (simple check)
                    if new_case.strip() in lines[i-1]: exists = True
                    
                    if not exists:
                        lines.insert(i, new_case)
                        return True
                    return False # Already exists or skipped
                
                if "}" in lines[i] and "switch" not in lines[i]: 
                    # Stop if we hit end of switch
                    return False
        return False

    # 2. Update Switch Statements
    # Update Update
    case_str = f"        case SCENE_{upper_name}: scene{class_name}Update(kDown, kHeld); break;\n"
    insert_case("void sceneManagerUpdate", "switch (currentScene)", case_str)

    # Update Render
    case_str = f"        case SCENE_{upper_name}: scene{class_name}Render(); break;\n"
    insert_case("void sceneManagerRender", "switch (currentScene)", case_str)

    # Update SwitchTo (Exit logic - 1st switch)
    # We manually scan for SwitchTo because it has two switches
    in_switch_to = False
    switch_count = 0
    
    # We need to iterate carefully because we are modifying the list
    # It's safer to find indices first, but for simplicity we'll do a robust scan
    
    # Let's re-read lines for this complex part or just patch carefully
    # Re-reading logic for SwitchTo specifically:
    
    # Find Exit Switch
    exit_switch_idx = -1
    init_switch_idx = -1
    
    for i, line in enumerate(lines):
        if "void sceneManagerSwitchTo" in line:
            in_switch_to = True
        if in_switch_to and "switch (currentScene)" in line:
            switch_count += 1
            if switch_count == 1: exit_switch_idx = i
            if switch_count == 2: init_switch_idx = i
            
    # Insert Exit case
    if exit_switch_idx != -1:
        case_str = f"        case SCENE_{upper_name}: scene{class_name}Exit(); break;\n"
        # Find insertion point after exit_switch_idx
        for j in range(exit_switch_idx, len(lines)):
            if "default:" in lines[j] or "}" in lines[j]:
                if case_str.strip() not in lines[j-1]: # Avoid duplicate
                    lines.insert(j, case_str)
                break

    # Recalculate indices because we inserted lines
    # Reset and find Init Switch again to be safe
    in_switch_to = False
    switch_count = 0
    init_switch_idx = -1
    for i, line in enumerate(lines):
        if "void sceneManagerSwitchTo" in line:
            in_switch_to = True
        if in_switch_to and "switch (currentScene)" in line:
            switch_count += 1
            if switch_count == 2: 
                init_switch_idx = i
                break
                
    # Insert Init case
    if init_switch_idx != -1:
        case_str = f"        case SCENE_{upper_name}: scene{class_name}Init(); break;\n"
        for j in range(init_switch_idx, len(lines)):
            if "default:" in lines[j] or "}" in lines[j]:
                if case_str.strip() not in lines[j-1]:
                    lines.insert(j, case_str)
                break

    with open(file_path, 'w') as f:
        f.writelines(lines)
    print(f"Updated logic in {file_path}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_scenes.py <scene_name1> <scene_name2> ...")
        return

    for scene_name in sys.argv[1:]:
        print(f"--- Processing {scene_name} ---")
        generate_files(scene_name)
        update_scene_types(scene_name)
        update_scene_manager(scene_name)
        print("-------------------------------")

if __name__ == "__main__":
    main()