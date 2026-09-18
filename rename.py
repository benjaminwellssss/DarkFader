import os, glob, re

replacements = [
    (r'\bslave\b', 'child'),
    (r'\bSlave\b', 'Child'),
    (r'\bSLAVE\b', 'CHILD'),
    (r'\bmaster\b', 'father'),
    (r'\bMaster\b', 'Father'),
    (r'\bMASTER\b', 'FATHER')
]

for filepath in glob.glob('Source/*.cpp') + glob.glob('Source/*.h'):
    with open(filepath, 'r') as f:
        content = f.read()
    
    new_content = content
    for pattern, replacement in replacements:
        new_content = re.sub(pattern, replacement, new_content)
        
    with open(filepath, 'w') as f:
        f.write(new_content)
    print(f"Processed {filepath}")
