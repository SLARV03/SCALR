import string

def validate_grammar(text: str) -> tuple[bool, str]:
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    if not lines:
        return False, "Grammar is empty."

    errors = []
    
    # Trackers for advanced validation
    defined_nonterminals = set()
    used_symbols = set()
    parsed_rules = [] # list of (LHS, [RHS parts])
    duplicate_detector = set()
    
    # 1. Base Checks & Duplicate Checks
    for i, line in enumerate(lines, 1):
        if '->' not in line:
            errors.append(f"Line {i}: Missing '->'. Got: {line}")
            continue
            
        lhs, rhs = (p.strip() for p in line.split('->', 1))
        
        if not lhs or not lhs[0].isupper():
            errors.append(f"Line {i}: LHS '{lhs}' must start with an uppercase letter.")
            continue
            
        if ' ' in lhs:
            errors.append(f"Line {i}: LHS '{lhs}' must be a single symbol without spaces.")
            continue
            
        if not rhs:
            errors.append(f"Line {i}: RHS is completely empty after '->'. If you meant epsilon, use 'epsilon' or 'ε'.")
            continue
            
        alternatives = [a.strip() for a in rhs.split('|')]
        if any(not a for a in alternatives):
            errors.append(f"Line {i}: Empty alternative in '{line}'. If you meant epsilon, use 'epsilon' or 'ε'.")
            continue
            
        defined_nonterminals.add(lhs)
        
        # Invalid Character check
        allowed_chars = set(string.printable + "ε")
        if any(c not in allowed_chars for c in line):
            errors.append(f"Line {i}: Contains invalid/unprintable characters.")
            continue
            
        for alt in alternatives:
            rule_str = f"{lhs} -> {alt}"
            if rule_str in duplicate_detector:
                errors.append(f"Line {i}: Duplicate rule detected: '{rule_str}'.")
            else:
                duplicate_detector.add(rule_str)
                parts = alt.split()
                parsed_rules.append((lhs, parts))
                for symbol in parts:
                    used_symbols.add(symbol)
                
    if errors:
        return False, format_errors(errors)

    if not parsed_rules:
        return False, "No valid production rules found."

    start_symbol = parsed_rules[0][0]

    # 2. Undefined Non-Terminals
    # Any symbol starting with uppercase used on the RHS must be defined.
    for sym in used_symbols:
        if sym[0].isupper() and sym not in defined_nonterminals:
            errors.append(f"Undefined Non-Terminal: '{sym}' is used on the right side but has no production rules.")
            
    # 3. Unreachable Non-Terminals
    # Breadth-first search from the start symbol
    reachable = set([start_symbol])
    queue = [start_symbol]
    
    # build adjacency list for reachability
    adj = {nt: set() for nt in defined_nonterminals}
    for lhs, rhs_parts in parsed_rules:
        for sym in rhs_parts:
            if sym[0].isupper() and sym in defined_nonterminals:
                adj[lhs].add(sym)
                
    while queue:
        current = queue.pop(0)
        for neighbor in adj[current]:
            if neighbor not in reachable:
                reachable.add(neighbor)
                queue.append(neighbor)
                
    for nt in defined_nonterminals:
        if nt not in reachable:
            errors.append(f"Unreachable Non-Terminal: '{nt}' can never be reached from the start symbol '{start_symbol}'.")
            
    # 4. Cyclic Productions
    # Build graph of unit productions (e.g. A -> B)
    unit_adj = {nt: [] for nt in defined_nonterminals}
    for lhs, rhs_parts in parsed_rules:
        if len(rhs_parts) == 1 and rhs_parts[0][0].isupper() and rhs_parts[0] in defined_nonterminals:
            unit_adj[lhs].append(rhs_parts[0])
            
    def has_cycle(node, visited, recursion_stack):
        visited.add(node)
        recursion_stack.add(node)
        
        for neighbor in unit_adj.get(node, []):
            if neighbor not in visited:
                if has_cycle(neighbor, visited, recursion_stack):
                    return True
            elif neighbor in recursion_stack:
                return True
                
        recursion_stack.remove(node)
        return False

    visited = set()
    for nt in defined_nonterminals:
        if nt not in visited:
            if has_cycle(nt, visited, set()):
                errors.append(f"Cyclic Production detected involving '{nt}'. (e.g. A -> B and B -> A)")

    if errors:
        return False, format_errors(errors)

    return True, ""

def format_errors(errors):
    msg = "\n".join(errors[:6])
    if len(errors) > 6:
        msg += f"\n...and {len(errors)-6} more error(s)."
    return msg
