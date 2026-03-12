#!/usr/bin/env python3
"""Clean up map_groups.h by removing duplicate definitions"""

with open('D:/Godot/pokefirered-master/include/constants/map_groups.h', 'r') as f:
    lines = f.readlines()

# Track which defines we've seen
seen = {}
cleaned = []

for line in lines:
    # Check if this is a #define line
    if line.startswith('#define MAP_'):
        parts = line.split()
        if len(parts) >= 2:
            name = parts[1]
            # Skip if we've seen this define before
            if name in seen:
                print(f"Skipping duplicate: {name}")
                continue
            seen[name] = True
    cleaned.append(line)

with open('D:/Godot/pokefirered-master/include/constants/map_groups.h', 'w') as f:
    f.writelines(cleaned)

print(f"Removed {len(lines) - len(cleaned)} duplicate definitions")
print(f"Final line count: {len(cleaned)}")
