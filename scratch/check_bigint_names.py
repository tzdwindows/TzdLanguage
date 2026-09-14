with open('TzdNativeRuntime.hpp', 'r', encoding='utf-8') as f:
    for idx, line in enumerate(f, 1):
        if 'bigint' in line.lower() and ('"' in line or 'inline' in line):
            print(f'{idx}: {line.strip()[:80]}')
