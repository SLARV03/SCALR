import customtkinter as ctk
import tkinter as tk
from tkinter import ttk
import json, subprocess, threading, os, sys, datetime, re

#APPEARANCE 
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("green")

#FONTS 
FONT_HEADING = ("Segoe UI", 26, "bold")
FONT_SUBHEAD = ("Segoe UI", 18, "bold")
FONT_BODY    = ("Segoe UI", 15)
FONT_SMALL   = ("Segoe UI", 13)
FONT_BTN     = ("Segoe UI", 14, "bold")
FONT_MONO    = ("Consolas", 13)

# THEME
BG       = "#0D1117"   # near-black canvas
PANEL    = "#161B22"   # card surface
PANEL2   = "#1C2430"   # slightly lighter card
BORDER   = "#21262D"   # subtle divider
ACCENT   = "#238636"   # GitHub-style green
ACCENT2  = "#2EA043"   # lighter green (hover)
ACCENT3  = "#3FB950"   # bright success green
TEXT     = "#E6EDF3"   # primary text
MUTED    = "#8B949E"   # secondary text
SUCCESS  = "#3FB950"
WARN     = "#D29922"
FAIL     = "#F85149"
TAB_ACT  = "#238636"
TAB_IN   = "#161B22"

PARSERS = ["LR0", "SLR1", "LALR1", "CLR1"]
def get_backend_path():
    if getattr(sys, 'frozen', False):
        # Running in a bundle (PyInstaller)
        base_path = sys._MEIPASS
    else:
        # Running in normal python environment
        base_path = os.path.dirname(os.path.abspath(__file__))
    
    return os.path.join(
        base_path,
        "scalr.exe" if sys.platform == "win32" else "scalr"
    )

BACKEND = get_backend_path()


# GRAMMAR VALIDATOR: imported from grammar_validator.py
from grammar_validator import validate_grammar, validate_input_string


# BACKEND 
def run_all_parsers(grammar: str, input_str: str) -> dict:
    try:
        full_input = grammar + "\n---\n" + input_str if input_str else grammar
        p = subprocess.run([BACKEND], input=full_input,
                           capture_output=True, text=True, timeout=15)
        out = p.stdout.strip()
        if not out:
            return {m: {"status": "error", "message": f"No output. stderr: {p.stderr.strip()}"} for m in PARSERS}
            
        all_data = json.loads(out)
        if all_data.get("status") != "success":
            return {m: {"status": "error", "message": all_data.get("message", "Unknown error")} for m in PARSERS}
            
        results = {}
        for m in PARSERS:
            parser_data = all_data.get("parsers", {}).get(m, {})
            # Transplant shared data into the individual dictionary to keep GUI compatibility
            full_parser_result = {
                "status": "success",
                "grammar_map": all_data.get("grammar_map"),
                "first_sets": all_data.get("first_sets"),
                "follow_sets": all_data.get("follow_sets"),
            }
            full_parser_result.update(parser_data)
            results[m] = full_parser_result
            
        return results
        
    except FileNotFoundError:
        return {m: {"status": "error", "message": f"Backend not found: {BACKEND}\nCompile: g++ -std=c++20 -o scalr src/*.cpp -I include"} for m in PARSERS}
    except subprocess.TimeoutExpired:
        return {m: {"status": "error", "message": "Backend timed out (>15s)"} for m in PARSERS}
    except json.JSONDecodeError as e:
        # Check if the output has non-json output before it
        msg = f"JSON error: {e}"
        if "\n" in out:
            msg += "\nRaw stdout:\n" + out
        return {m: {"status": "error", "message": msg} for m in PARSERS}
    except Exception as e:
        return {m: {"status": "error", "message": str(e)} for m in PARSERS}


def run_single_parser(grammar: str, input_str: str, method: str) -> dict:
    """Run backend and extract only one parser's result."""
    all_results = run_all_parsers(grammar, input_str)
    return {method: all_results.get(method, {"status": "error", "message": f"No data for {method}"})}


#  SHARED UI HELPERS 
def section_label(parent, text):
    bar = ctk.CTkFrame(parent, fg_color=ACCENT, corner_radius=6)
    bar.pack(fill="x", padx=10, pady=(12, 4))
    ctk.CTkLabel(bar, text=text, font=FONT_SUBHEAD, text_color="#FFFFFF").pack(
        anchor="w", padx=12, pady=6)

def error_label(parent, text):
    """Show an error banner instead of content."""
    f = ctk.CTkFrame(parent, fg_color=PANEL2, corner_radius=8,
                     border_width=1, border_color=FAIL)
    f.pack(fill="x", padx=10, pady=20)
    ctk.CTkLabel(f, text=f"✗  {text}", font=FONT_BODY,
                 text_color=FAIL, wraplength=860, justify="left").pack(padx=16, pady=14)

def placeholder_label(parent, text):
    ctk.CTkLabel(parent, text=text, font=FONT_BODY, text_color=MUTED).pack(pady=40)

def make_treeview(parent, columns, height=8):
    style = ttk.Style()
    style.theme_use("clam")
    style.configure("S.Treeview",
        background=PANEL, foreground=TEXT, rowheight=48,
        fieldbackground=PANEL, font=("Consolas", 18))
    style.configure("S.Treeview.Heading",
        background=ACCENT, foreground="white",
        font=("Segoe UI", 11, "bold"), relief="flat")
    style.map("S.Treeview",
        background=[("selected", ACCENT2)], foreground=[("selected", "white")])

    outer = ctk.CTkFrame(parent, fg_color=PANEL, corner_radius=8,
                         border_width=1, border_color=BORDER)
    outer.pack(fill="both", expand=True, padx=10, pady=4)
    tv  = ttk.Treeview(outer, columns=columns, show="headings",
                       height=height, style="S.Treeview")
    vsb = ttk.Scrollbar(outer, orient="vertical",   command=tv.yview)
    hsb = ttk.Scrollbar(outer, orient="horizontal", command=tv.xview)
    tv.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
    for col in columns:
        tv.heading(col, text=col)
        tv.column(col, anchor="center", width=max(120, len(col) * 13))
    tv.grid(row=0, column=0, sticky="nsew")
    vsb.grid(row=0, column=1, sticky="ns")
    hsb.grid(row=1, column=0, sticky="ew")
    outer.grid_rowconfigure(0, weight=1)
    outer.grid_columnconfigure(0, weight=1)
    return tv

def bar_chart(parent, title, data: dict, color):
    frame = ctk.CTkFrame(parent, fg_color=PANEL, corner_radius=8,
                         border_width=1, border_color=BORDER)
    frame.pack(fill="x", padx=10, pady=6)
    ctk.CTkLabel(frame, text=title, font=FONT_SUBHEAD, text_color=TEXT).pack(
        anchor="w", padx=14, pady=(10, 4))
    cv = tk.Canvas(frame, height=130, bg=PANEL, highlightthickness=0)
    cv.pack(fill="x", padx=14, pady=(0, 12))
    cv.update_idletasks()
    W = cv.winfo_width() or 700
    values  = list(data.values())
    max_val = max(values) if any(v > 0 for v in values) else 1
    bar_h, gap, label_w, val_w = 22, 8, 72, 54
    chart_w = W - label_w - val_w - 20
    for i, (method, val) in enumerate(data.items()):
        y = 10 + i * (bar_h + gap)
        cv.create_text(label_w - 5, y + bar_h // 2, text=method,
                       anchor="e", font=("Consolas", 16, "bold"), fill=MUTED)
        cv.create_rectangle(label_w, y, label_w + chart_w, y + bar_h,
                            fill=PANEL2, outline="")
        filled = int((val / max_val) * chart_w) if max_val > 0 else 0
        if filled > 0:
            cv.create_rectangle(label_w, y, label_w + filled, y + bar_h,
                                fill=color, outline="")
        cv.create_text(label_w + chart_w + 6, y + bar_h // 2,
                       text=str(val), anchor="w", font=("Consolas", 16), fill=TEXT)


# MAIN APP 
class ScalrApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("SCALR — Grammar Tester for LR Parsing")
        self.geometry("1100x760")
        self.minsize(900, 600)
        self.configure(fg_color=BG)
        self.results = {}
        self.grammar = ""
        self.selected_parser = "All Parsers"   # dropdown state
        self._build()

    #  LAYOUT 
    def _build(self):
        # Header
        hdr = ctk.CTkFrame(self, fg_color="#0A0E14", corner_radius=0, height=58)
        hdr.pack(fill="x")
        hdr.pack_propagate(False)
        ctk.CTkLabel(hdr, text="SCALR", font=("Consolas", 22, "bold"),
                     text_color=ACCENT3).pack(side="left", padx=22, pady=14)
        ctk.CTkLabel(hdr, text="Grammar Tester  ·  LR Parsing Analysis",
                     font=FONT_SMALL, text_color=MUTED).pack(side="left")

        # Neon accent line
        ctk.CTkFrame(self, fg_color=ACCENT, corner_radius=0, height=2).pack(fill="x")

        # Tab bar
        tab_bar = ctk.CTkFrame(self, fg_color="#0D1117", corner_radius=0, height=46)
        tab_bar.pack(fill="x")
        tab_bar.pack_propagate(False)
        self.tab_btns = {}
        self.ALL_TABS = ["Editor", "Results", "Analytics", "Detailed View", "Parse Tree"]
        for label in self.ALL_TABS:
            btn = ctk.CTkButton(
                tab_bar, text=label, font=FONT_BTN,
                fg_color=TAB_IN, text_color=MUTED,
                hover_color="#1C2430", corner_radius=0,
                border_width=0, height=46,
                command=lambda l=label: self._switch_tab(l))
            btn.pack(side="left", padx=1)
            self.tab_btns[label] = btn

        # Content
        self.content = ctk.CTkFrame(self, fg_color=BG, corner_radius=0)
        self.content.pack(fill="both", expand=True)

        self.pages = {
            "Editor":        self._build_editor(self.content),
            "Results":       self._build_d1(self.content),
            "Analytics":     self._build_d2(self.content),
            "Detailed View": self._build_d3(self.content),
            "Parse Tree":    self._build_parse_tree(self.content),
        }
        self._switch_tab("Editor")

    def _switch_tab(self, label):
        # Determine which tabs are restricted
        restricted = self._get_restricted_tabs()
        if label in restricted:
            return  # block navigation to restricted tabs

        for page in self.pages.values():
            page.pack_forget()
        self.pages[label].pack(fill="both", expand=True)
        for name, btn in self.tab_btns.items():
            active = name == label
            is_restricted = name in restricted
            if is_restricted:
                btn.configure(
                    fg_color="#0D1117", text_color="#3D444D")  # greyed-out look
            else:
                btn.configure(
                    fg_color=TAB_ACT if active else TAB_IN,
                    text_color=TEXT   if active else MUTED)

    def _get_restricted_tabs(self):
        """Return set of tab names that should be inaccessible given the current parser selection."""
        if self.selected_parser == "All Parsers":
            return set()
        # When a specific parser is chosen, Results & Analytics are comparison dashboards — disable them
        return {"Results", "Analytics"}

    def _on_parser_dropdown_changed(self, choice):
        self.selected_parser = choice
        restricted = self._get_restricted_tabs()
        # If current page is now restricted, bounce back to Editor
        for name, page in self.pages.items():
            if page.winfo_ismapped() and name in restricted:
                self._switch_tab("Editor")
                return
        # Refresh tab button appearances
        self._switch_tab(self._get_current_tab())

    def _get_current_tab(self):
        for name, page in self.pages.items():
            if page.winfo_ismapped():
                return name
        return "Editor"

    #  EDITOR PAGE 
    def _build_editor(self, parent):
        page  = ctk.CTkFrame(parent, fg_color=BG, corner_radius=0)
        inner = ctk.CTkFrame(page, fg_color=BG)
        inner.pack(fill="both", expand=True, padx=28, pady=20)

        ctk.CTkLabel(inner, text="Context-Free Grammar",
                     font=FONT_SUBHEAD, text_color=ACCENT3).pack(anchor="w")
        ctk.CTkLabel(inner, text="One production per line.  Format:  LHS -> sym1 sym2 ...  |  alt",
                     font=FONT_SMALL, text_color=MUTED).pack(anchor="w", pady=(2, 10))

        self.editor = ctk.CTkTextbox(
            inner, font=FONT_MONO, corner_radius=8, height=180,
            border_width=1, border_color=BORDER,
            fg_color=PANEL, text_color=TEXT)
        self.editor.pack(fill="x")
        self.editor.insert("1.0",
            "S -> E\nE -> E + T\nE -> T\nT -> T * F\nT -> F\nF -> ( E )\nF -> id")

        ctk.CTkLabel(inner, text="Input String to Parse (Optional)",
                     font=FONT_SUBHEAD, text_color=ACCENT3).pack(anchor="w", pady=(15, 0))
        self.input_editor = ctk.CTkTextbox(
            inner, font=FONT_MONO, corner_radius=8, height=40,
            border_width=1, border_color=BORDER,
            fg_color=PANEL, text_color=TEXT)
        self.input_editor.pack(fill="x", pady=(2, 10))
        self.input_editor.insert("1.0", "id + id * id")

        self.status_var = tk.StringVar(value="")
        ctk.CTkLabel(inner, textvariable=self.status_var,
                     font=FONT_SMALL, text_color=WARN).pack(anchor="w", pady=(6, 0))

        btn_row = ctk.CTkFrame(inner, fg_color=BG)
        btn_row.pack(fill="x", pady=10)

        # Parser generator dropdown
        parser_choices = ["All Parsers"] + PARSERS
        self.parser_dropdown = ctk.CTkOptionMenu(
            btn_row, values=parser_choices,
            font=FONT_BTN, dropdown_font=FONT_BODY,
            fg_color=PANEL2, button_color=ACCENT,
            button_hover_color=ACCENT2,
            text_color=TEXT, dropdown_fg_color=PANEL,
            dropdown_text_color=TEXT, dropdown_hover_color=ACCENT2,
            corner_radius=8, height=40, width=160,
            command=self._on_parser_dropdown_changed)
        self.parser_dropdown.set("All Parsers")
        self.parser_dropdown.pack(side="left", padx=(0, 10))

        self.run_btn = ctk.CTkButton(
            btn_row, text="▶  Analyse Grammar", font=FONT_BTN,
            fg_color=ACCENT, hover_color=ACCENT2,
            corner_radius=8, height=40, command=self._submit)
        self.run_btn.pack(side="left", padx=(0, 10))
        ctk.CTkButton(btn_row, text="Clear", font=FONT_BTN,
                      fg_color=PANEL2, text_color=MUTED, hover_color=BORDER,
                      corner_radius=8, height=40,
                      command=lambda: [self.editor.delete("1.0", "end"), self.input_editor.delete("1.0", "end")]).pack(side="left")

        self.prog = ctk.CTkProgressBar(inner, mode="indeterminate",
                                       fg_color=BORDER, progress_color=ACCENT3)

        # Log terminal
        log_hdr = ctk.CTkFrame(inner, fg_color=PANEL, corner_radius=6,
                                border_width=1, border_color=BORDER)
        log_hdr.pack(fill="x", pady=(14, 0))
        ctk.CTkLabel(log_hdr, text="▣  Output Log",
                     font=("Consolas", 12, "bold"),
                     text_color=ACCENT3).pack(side="left", padx=12, pady=5)
        ctk.CTkButton(log_hdr, text="Clear Log", font=FONT_SMALL,
                      fg_color=PANEL2, hover_color=BORDER,
                      height=26, corner_radius=4,
                      command=self._clear_log).pack(side="right", padx=8, pady=4)

        self.log_box = ctk.CTkTextbox(
            inner, font=("Consolas", 12), corner_radius=0, height=220,
            fg_color="#060A0F", text_color="#C9D1D9",
            border_width=1, border_color=BORDER, wrap="word", state="disabled")
        self.log_box.pack(fill="x", pady=(0, 10))
        return page

    def _log(self, message: str, level: str = "info"):
        ts = datetime.datetime.now().strftime("%H:%M:%S")
        colours = {"info": "#8B949E", "ok": "#3FB950", "warn": "#D29922", "error": "#F85149"}
        prefixes = {"info": "  ", "ok": "✓ ", "warn": "⚠ ", "error": "✗ "}
        colour, prefix = colours.get(level, "#C9D1D9"), prefixes.get(level, "  ")
        line = f"[{ts}] {prefix}{message}\n"
        self.log_box.configure(state="normal")
        self.log_box.insert("end", line)
        self.log_box._textbox.tag_add(level, "end - 2 lines", "end - 1 lines")
        self.log_box._textbox.tag_configure(level, foreground=colour)
        self.log_box.see("end")
        self.log_box.configure(state="disabled")

    def _clear_log(self):
        self.log_box.configure(state="normal")
        self.log_box.delete("1.0", "end")
        self.log_box.configure(state="disabled")

    #  SUBMIT & BACKGROUND 
    def _submit(self):
        grammar = self.editor.get("1.0", "end").strip()
        input_str = self.input_editor.get("1.0", "end").strip()
        if not grammar:
            self.status_var.set("⚠  Please enter a grammar.")
            self._log("No grammar entered.", "warn")
            return

        valid, err_msg = validate_grammar(grammar)
        if not valid:
            self._log("─" * 50)
            self._log("Grammar validation failed:", "error")
            for ln in err_msg.splitlines():
                if ln.strip():
                    self._log("  " + ln.strip(), "error")
            self.status_var.set("✗  Grammar or Input String has format errors — see log below.")
            # Show error state on all dashboards immediately
            self._show_grammar_error(err_msg)
            return

        # Validate input string terminals
        if input_str:
            valid_input, input_err = validate_input_string(grammar, input_str)
            if not valid_input:
                self._log("─" * 50)
                self._log("Input string validation failed:", "error")
                for ln in input_err.splitlines():
                    if ln.strip():
                        self._log("  " + ln.strip(), "error")
                self.status_var.set("✗  Input string has invalid tokens — see log below.")
                self._show_grammar_error(input_err)
                return

        self._log("─" * 50)
        selected = self.selected_parser
        if selected == "All Parsers":
            self._log("Grammar accepted. Running all 4 parsers...", "info")
            self.status_var.set("Running all parsers…")
        else:
            self._log(f"Grammar accepted. Running {selected} parser...", "info")
            self.status_var.set(f"Running {selected}…")
        self.grammar = grammar
        self.run_btn.configure(state="disabled")
        self.prog.pack(fill="x", pady=4)
        self.prog.start()
        threading.Thread(target=self._bg, args=(grammar, input_str), daemon=True).start()

    def _show_grammar_error(self, err_msg: str):
        """Populate all dashboards with a grammar-error banner."""
        short = err_msg.splitlines()[0] if err_msg else "Grammar format error"
        banner = f"Grammar error — fix your grammar in the Editor tab.\n{short}"
        for attr in ("d1_scroll", "d2_scroll", "d3_scroll", "pt_scroll"):
            sc = getattr(self, attr, None)
            if sc is None:
                continue
            for w in sc.winfo_children():
                w.destroy()
            error_label(sc, banner)

    def _bg(self, grammar, input_str):
        selected = self.selected_parser
        if selected == "All Parsers":
            result = run_all_parsers(grammar, input_str)
        else:
            result = run_single_parser(grammar, input_str, selected)
        self.after(0, self._done, result)

    def _done(self, results):
        self.results = results
        self.prog.stop(); self.prog.pack_forget()
        self.run_btn.configure(state="normal")

        # Binary missing: handle error
        for r in results.values():
            if r.get("status") == "error" and "not found" in r.get("message", "").lower():
                self._log("Backend binary not found!", "error")
                self._log(f"Expected: {BACKEND}", "error")
                self._log("Fix: g++ -std=c++20 -o scalr src/*.cpp -I include", "warn")
                self.status_var.set("✗  Backend binary missing.")
                return

        for method, r in results.items():
            if r.get("status") == "error":
                msg = r.get("message", "")
                lvl = "warn" if "timed out" in msg else "error"
                self._log(f"[{method}] {msg}", lvl)
            else:
                m = r.get("meta", {})
                c = m.get("conflicts", "?")
                self._log(f"[{method}]  states={m.get('states','?')}  "
                          f"conflicts={c}  time={m.get('time_ms',0):.3f}ms",
                          "ok" if c == 0 else "warn")

        self._log("Analysis complete.", "ok")
        selected = self.selected_parser
        if selected == "All Parsers":
            self.status_var.set("✓  Done — see Results / Analytics / Detailed View / Parse Tree.")
            self._refresh_d1(); self._refresh_d2(); self._refresh_d3(); self._refresh_parse_tree()
        else:
            self.status_var.set(f"✓  Done — see Detailed View / Parse Tree for {selected}.")
            # Only refresh the single-parser dashboards, set the selectors to the chosen parser
            self.d3_sel.set(selected)
            self.pt_sel.set(selected)
            self._refresh_d3(); self._refresh_parse_tree()
            # Clear comparison dashboards
            for attr in ("d1_scroll", "d2_scroll"):
                sc = getattr(self, attr, None)
                if sc:
                    for w in sc.winfo_children(): w.destroy()
                    placeholder_label(sc, f"Comparison dashboards are disabled in single-parser mode ({selected}).")

    #  RESULTS PAGE (Dashboard1)
    def _build_d1(self, parent):
        page = ctk.CTkFrame(parent, fg_color=BG, corner_radius=0)
        self.d1_scroll = ctk.CTkScrollableFrame(page, fg_color=BG)
        self.d1_scroll.pack(fill="both", expand=True)
        placeholder_label(self.d1_scroll, "Run a grammar from the Editor tab to see results.")
        return page

    def _refresh_d1(self):
        sc = self.d1_scroll
        for w in sc.winfo_children(): w.destroy()

        section_label(sc, "Grammar Input")
        gb = ctk.CTkTextbox(sc, font=FONT_MONO, height=80, corner_radius=6,
                            fg_color=PANEL, text_color=TEXT,
                            border_width=1, border_color=BORDER)
        gb.pack(fill="x", padx=10, pady=4)
        gb.insert("1.0", self.grammar)
        gb.configure(state="disabled")

        section_label(sc, "Parser Compatibility Summary")
        tv = make_treeview(sc, ("Parser", "Supported", "States", "Conflicts", "Time (ms)"), 5)
        for m in PARSERS:
            r = self.results.get(m, {})
            if r.get("status") == "error":
                tv.insert("", "end", values=(m, "ERROR", "-", "-", "-"))
                continue
            meta = r.get("meta", {}); c = meta.get("conflicts", "-")
            sup  = "✓ Yes" if c == 0 else f"⚠ {c} conflict(s)"
            tag  = "conflict" if isinstance(c, int) and c > 0 else ""
            tv.insert("", "end", values=(
                m, sup, meta.get("states", "-"), c,
                f"{meta.get('time_ms', 0):.3f}"), tags=(tag,))
            tv.tag_configure("conflict", background="#2D1B1B", foreground=FAIL)

        section_label(sc, "Recommendation")
        rec, reason = self._recommend()
        rec_f = ctk.CTkFrame(sc, fg_color=PANEL2, corner_radius=8,
                             border_width=1, border_color=ACCENT)
        rec_f.pack(fill="x", padx=10, pady=6)
        ctk.CTkLabel(rec_f, text=rec, font=("Consolas", 16, "bold"),
                     text_color=ACCENT3).pack(anchor="w", padx=16, pady=(12, 2))
        ctk.CTkLabel(rec_f, text=reason, font=FONT_SMALL, text_color=MUTED,
                     wraplength=900, justify="left").pack(anchor="w", padx=16, pady=(0, 12))

    def _recommend(self):
        desc = {
            "LR0":   "LR(0) handles this grammar without conflicts — the simplest parser suffices.",
            "SLR1":  "SLR(1) resolves all conflicts using basic FOLLOW sets.",
            "LALR1": "LALR(1) is recommended — resolves conflicts with merged lookaheads; used by YACC/Bison.",
            "CLR1":  "CLR(1) is required — only the canonical LR(1) parser handles this grammar conflict-free.",
        }
        for m in PARSERS:
            if (self.results.get(m, {}).get("status") == "success"
                    and self.results[m].get("meta", {}).get("conflicts", 1) == 0):
                return m, desc[m]
        return "None", "No LR parser handles this grammar without conflicts. It may be inherently ambiguous."

    #  ANALYTICS PAGE (Dashboard2) 
    def _build_d2(self, parent):
        page = ctk.CTkFrame(parent, fg_color=BG, corner_radius=0)
        self.d2_scroll = ctk.CTkScrollableFrame(page, fg_color=BG)
        self.d2_scroll.pack(fill="both", expand=True)
        placeholder_label(self.d2_scroll, "Run a grammar from the Editor tab to see analytics.")
        return page

    def _refresh_d2(self):
        sc = self.d2_scroll
        for w in sc.winfo_children(): w.destroy()

        section_label(sc, "Metrics Comparison")
        tv = make_treeview(sc, ("Metric", "LR0", "SLR1", "LALR1", "CLR1"), 6)

        def get(m, k):
            r = self.results.get(m, {})
            return r.get("meta", {}).get(k, "-") if r.get("status") == "success" else "ERR"
        def cnt(m, t):
            r = self.results.get(m, {})
            return sum(1 for c in r.get("conflicts", []) if c.get("type") == t) \
                   if r.get("status") == "success" else "ERR"

        for label, fn in [
            ("States",         lambda m: get(m, "states")),
            ("Total Conflicts", lambda m: get(m, "conflicts")),
            ("S/R Conflicts",  lambda m: cnt(m, "S/R")),
            ("R/R Conflicts",  lambda m: cnt(m, "R/R")),
            ("Time (ms)",      lambda m: f"{self.results[m].get('meta',{}).get('time_ms',0):.3f}"
                                         if self.results[m].get("status") == "success" else "ERR"),
        ]:
            tv.insert("", "end", values=(label,) + tuple(fn(m) for m in PARSERS))

        section_label(sc, "Visual Comparison")
        for title, key, color in [
            ("Conflicts per Parser", "conflicts", FAIL),
            ("States per Parser",   "states",    ACCENT),
            ("Time (ms) per Parser","time_ms",   ACCENT2),
        ]:
            data = {m: round(self.results[m].get("meta", {}).get(key, 0), 3) for m in PARSERS}
            bar_chart(sc, title, data, color)

    #  DETAILED VIEW PAGE:parse tabke (Dashboard3) 
    def _build_d3(self, parent):
        page = ctk.CTkFrame(parent, fg_color=BG, corner_radius=0)
        sel  = ctk.CTkFrame(page, fg_color=PANEL, corner_radius=0, height=46)
        sel.pack(fill="x"); sel.pack_propagate(False)
        self.d3_sel = tk.StringVar(value="SLR1")
        for m in PARSERS:
            ctk.CTkRadioButton(sel, text=m, variable=self.d3_sel, value=m,
                               font=FONT_BTN, fg_color=ACCENT,
                               command=self._refresh_d3).pack(side="left", padx=16, pady=10)
        self.d3_scroll = ctk.CTkScrollableFrame(page, fg_color=BG)
        self.d3_scroll.pack(fill="both", expand=True)
        placeholder_label(self.d3_scroll, "Run a grammar from the Editor tab to see detailed tables.")
        return page

    def _refresh_d3(self):
        sc = self.d3_scroll
        for w in sc.winfo_children(): w.destroy()
        if not self.results:
            placeholder_label(sc, "Run a grammar from the Editor tab first.")
            return

        method = self.d3_sel.get()
        r = self.results.get(method, {})
        if r.get("status") == "error":
            error_label(sc, f"[{method}] {r.get('message', 'Unknown error')}")
            return

        section_label(sc, f"Grammar Rules  [{method}]")
        gmap = r.get("grammar_map", [])
        tv = make_treeview(sc, ("ID", "Rule", "Source Line"), min(len(gmap)+1, 7))
        for rule in gmap:
            tv.insert("", "end", values=(rule["id"], rule["rule"], rule["line"]))

        section_label(sc, f"FIRST and FOLLOW Sets  [{method}]")
        first_sets  = r.get("first_sets",  {})
        follow_sets = r.get("follow_sets", {})
        syms = sorted(set(list(first_sets) + list(follow_sets)))
        tv2  = make_treeview(sc, ("Non-Terminal", "FIRST", "FOLLOW"), min(len(syms)+1, 8))
        for sym in syms:
            tv2.insert("", "end", values=(
                sym,
                "{ " + ", ".join(sorted(first_sets.get(sym,  []))) + " }",
                "{ " + ", ".join(sorted(follow_sets.get(sym, []))) + " }"))

        section_label(sc, f"Parsing Table  [{method}]")
        table    = r.get("table", {})
        all_syms = sorted({sym for sd in table.values() for sym in sd})
        if all_syms:
           tv3 = make_treeview(sc, ["State"] + all_syms, min(len(table) + 1, 14))
           tv3.tag_configure("has_conflict", background="#2D1010", foreground=FAIL)

           for sk, sd in sorted(table.items(), key=lambda x: int(x[0].split("_")[1])):
               row = [sk.replace("state_", "")] + [
                      ", ".join(sd.get(s, ["-"])) for s in all_syms
                 ]

        # A cell is a conflict if it has multiple actions (comma present)
               is_conflict = any("," in cell for cell in row[1:])
               tag = ("has_conflict",) if is_conflict else ()

               tv3.insert("", "end", values=tuple(row), tags=tag)

        else:
            placeholder_label(sc, "No table data.")

        section_label(sc, f"Conflicts  [{method}]")
        conflicts = r.get("conflicts", [])
        if not conflicts:
            ok = ctk.CTkFrame(sc, fg_color="#0D2218", corner_radius=8,
                              border_width=1, border_color=SUCCESS)
            ok.pack(fill="x", padx=10, pady=6)
            ctk.CTkLabel(ok, text=f"✓  No conflicts — grammar is fully supported by {method}",
                         font=FONT_BODY, text_color=SUCCESS).pack(padx=16, pady=12)
        else:
            tv4 = make_treeview(sc, ("Type", "State", "Symbol", "Involved Rules"),
                                min(len(conflicts)+1, 8))
            for c in conflicts:
                tv4.insert("", "end",
                           values=(c["type"], c["state"], c["symbol"],
                                   ", ".join(str(x) for x in c.get("rules", []))))
            section_label(sc, "Conflicts — Raw JSON")
            jb = ctk.CTkTextbox(sc, font=FONT_MONO, height=150, corner_radius=6,
                                fg_color=PANEL, text_color=TEXT,
                                border_width=1, border_color=BORDER)
            jb.pack(fill="x", padx=10, pady=4)
            jb.insert("1.0", json.dumps(conflicts, indent=2))
            jb.configure(state="disabled")

        parse_trace = r.get("parse_trace")
        if parse_trace:
            section_label(sc, f"Input Parsing Trace  [{method}]")
            accepted = parse_trace.get("accepted", False)
            error_msg = parse_trace.get("error", "")
            
            status_f = ctk.CTkFrame(sc, fg_color="#0D2218" if accepted else "#2D1010", corner_radius=8,
                                    border_width=1, border_color=SUCCESS if accepted else FAIL)
            status_f.pack(fill="x", padx=10, pady=6)
            status_text = "✓  Input Accepted" if accepted else f"✗  Input Rejected: {error_msg}"
            ctk.CTkLabel(status_f, text=status_text,
                         font=FONT_BODY, text_color=SUCCESS if accepted else FAIL).pack(padx=16, pady=12)
                         
            steps = parse_trace.get("steps", [])
            if steps:
                tv_trace = make_treeview(sc, ("Stack", "Input", "Action"), min(len(steps)+1, 12))
                tv_trace.tag_configure("error", background="#2D1010", foreground=FAIL)
                tv_trace.tag_configure("accept", background="#0D2218", foreground=SUCCESS)
                for step in steps:
                    action = step.get("action", "")
                    tag = ()
                    if "Error" in action or "Conflict" in action:
                        tag = ("error",)
                    elif action == "Accept":
                        tag = ("accept",)
                    tv_trace.insert("", "end", values=(step.get("stack", ""), step.get("input", ""), action), tags=tag)

        examples = r.get("example_strings", [])
        if examples:
            section_label(sc, f"Example Generated Strings  [{method}]")
            tv_ex = make_treeview(sc, ("String", "Type", "Description"), min(len(examples)+1, 6))
            tv_ex.tag_configure("conflict", background="#2D1010", foreground=FAIL)
            tv_ex.tag_configure("normal", background="#0D2218", foreground=SUCCESS)
            for ex in examples:
                ex_type = ex.get("type", "normal")
                tag = ("conflict",) if ex_type == "conflict" else ("normal",)
                tv_ex.insert("", "end", values=(ex.get("string", ""), ex_type, ex.get("description", "")), tags=tag)

    #  PARSE TREE PAGE 
    def _build_parse_tree(self, parent):
        page = ctk.CTkFrame(parent, fg_color=BG, corner_radius=0)
        sel  = ctk.CTkFrame(page, fg_color=PANEL, corner_radius=0, height=46)
        sel.pack(fill="x"); sel.pack_propagate(False)
        self.pt_sel = tk.StringVar(value="SLR1")
        for m in PARSERS:
            ctk.CTkRadioButton(sel, text=m, variable=self.pt_sel, value=m,
                               font=FONT_BTN, fg_color=ACCENT,
                               command=self._refresh_parse_tree).pack(side="left", padx=16, pady=10)
        
        # Using a frame to hold canvas and scrollbars for X and Y panning
        self.pt_scroll = ctk.CTkFrame(page, fg_color=BG)
        self.pt_scroll.pack(fill="both", expand=True)
        placeholder_label(self.pt_scroll, "Run a grammar from the Editor tab to see the parse tree.")
        return page

    def _refresh_parse_tree(self):
        sc = self.pt_scroll
        for w in sc.winfo_children(): w.destroy()
        if not getattr(self, 'results', None):
            placeholder_label(sc, "Run a grammar from the Editor tab first.")
            return

        method = self.pt_sel.get()
        r = self.results.get(method, {})
        if r.get("status") == "error":
            error_label(sc, f"[{method}] {r.get('message', 'Unknown error')}")
            return
            
        parse_trace = r.get("parse_trace")
        if not parse_trace or not parse_trace.get("accepted"):
            placeholder_label(sc, f"No parse tree available.\nInput string was rejected or missing for {method}.")
            return
            
        steps = parse_trace.get("steps", [])
        
        # HEATMAP ALGORITHM
        token_heat = []
        current_token = None
        current_reduces = 0
        
        for step in steps:
            action = step.get("action", "")
            input_rem = step.get("input", "")
            
            if action.startswith("Shift"):
                if current_token is not None:
                    token_heat.append({"token": current_token, "heat": current_reduces, "status": "ok"})
                
                tokens = input_rem.split()
                if tokens:
                    current_token = tokens[0]
                    current_reduces = 0
            
            elif action.startswith("Reduce"):
                current_reduces += 1
                
            elif "Conflict" in action or "Error" in action:
                if current_token is not None:
                    token_heat.append({"token": current_token, "heat": current_reduces, "status": "error"})
                current_token = None
            
            elif action == "Accept":
                if current_token is not None:
                    token_heat.append({"token": current_token, "heat": current_reduces, "status": "ok"})
                current_token = None
                
        if current_token is not None:
            token_heat.append({"token": current_token, "heat": current_reduces, "status": "error"})
            
        # RENDER HEATMAP
        if token_heat:
            section_label(sc, f"Parsing Heatmap (Reductions per Token)  [{method}]")
            hm_frame = ctk.CTkFrame(sc, fg_color=PANEL, corner_radius=8, border_width=1, border_color=BORDER)
            hm_frame.pack(fill="x", padx=10, pady=6)
            
            max_heat = max(1, max(item["heat"] for item in token_heat))
            
            def get_heat_color(heat, status):
                if status == "error": return "#800020" # Dark Red/Burgundy for error
                ratio = min(1.0, heat / max_heat)
                r = int(46 + (248 - 46) * ratio)
                g = int(160 + (81 - 160) * ratio)
                b = int(67 + (73 - 67) * ratio)
                return f"#{r:02x}{g:02x}{b:02x}"

            # We use a horizontal scrollable frame so long strings don't get cut off
            hm_scroll = ctk.CTkScrollableFrame(hm_frame, fg_color=PANEL, height=70, orientation="horizontal")
            hm_scroll.pack(fill="x", expand=True, padx=4, pady=4)
            
            for item in token_heat:
                color = get_heat_color(item["heat"], item["status"])
                border = FAIL if item["status"] == "error" else color
                bw = 2 if item["status"] == "error" else 0
                
                node_f = ctk.CTkFrame(hm_scroll, fg_color=color, corner_radius=6, border_width=bw, border_color=border)
                node_f.pack(side="left", padx=4, pady=4)
                
                ctk.CTkLabel(node_f, text=item["token"], font=("Consolas", 14, "bold"), text_color="#FFFFFF").pack(padx=10, pady=(6,0))
                ctk.CTkLabel(node_f, text=f"{item['heat']} reductions", font=("Consolas", 10), text_color="#E6EDF3").pack(padx=10, pady=(0,6))
        
        # TREE ALGORITHM
        stack = []
        for step in steps:
            action = step.get("action", "")
            input_rem = step.get("input", "")
            if action.startswith("Shift"):
                tokens = input_rem.split()
                if tokens:
                    # Token shifted is the first token currently in lookahead
                    stack.append(ParseTreeNode(tokens[0]))
            elif action.startswith("Reduce by "):
                prod = action.replace("Reduce by ", "")
                if " -> " in prod:
                    lhs, rhs = prod.split(" -> ", 1)
                    rhs_len = 0 if rhs in ("epsilon", "ε") else len(rhs.split())
                    children = []
                    if rhs in ("epsilon", "ε"):
                        children = [ParseTreeNode("ε")]
                    else:
                        for _ in range(rhs_len):
                            if stack: children.append(stack.pop())
                        children.reverse()
                    stack.append(ParseTreeNode(lhs, children))
        
        if not stack:
            placeholder_label(sc, "Failed to build tree from parse trace steps.")
            return
            
        root = stack[-1] # the final S node
        
        outer = ctk.CTkFrame(sc, fg_color=PANEL, corner_radius=8, border_width=1, border_color=BORDER)
        outer.pack(fill="both", expand=True, padx=10, pady=10)
        
        cv = tk.Canvas(outer, bg=PANEL, highlightthickness=0)
        vsb = ttk.Scrollbar(outer, orient="vertical", command=cv.yview)
        hsb = ttk.Scrollbar(outer, orient="horizontal", command=cv.xview)
        cv.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
        
        cv.grid(row=0, column=0, sticky="nsew")
        vsb.grid(row=0, column=1, sticky="ns")
        hsb.grid(row=1, column=0, sticky="ew")
        outer.grid_rowconfigure(0, weight=1)
        outer.grid_columnconfigure(0, weight=1)
        
        def calculate_positions(node, depth, x_offset):
            node.y = depth * 70 + 40
            if not node.children:
                node.x = x_offset + node.width() / 2
                return x_offset + node.width() + 10
            
            cur_x = x_offset
            for c in node.children:
                cur_x = calculate_positions(c, depth + 1, cur_x)
                
            node.x = (node.children[0].x + node.children[-1].x) / 2
            return cur_x
            
        calculate_positions(root, 0, 20)
        
        def get_max_bounds(node, mx, my):
            mx = max(mx, node.x)
            my = max(my, node.y)
            for c in node.children:
                mx, my = get_max_bounds(c, mx, my)
            return mx, my
        
        mx, my = get_max_bounds(root, 0, 0)
        cv.configure(scrollregion=(0, 0, mx + 60, my + 60))
        
        def draw_tree(node):
            if not node: return
            for c in node.children:
                cv.create_line(node.x, node.y + 15, c.x, c.y - 15, fill=MUTED, width=1.5)
                draw_tree(c)
                
            color = "#0D1117" if node.children else "#161B22"
            outline = ACCENT if not node.children else MUTED
            text_color = ACCENT3 if not node.children else TEXT
            
            cv.create_rectangle(node.x - 20, node.y - 16, node.x + 20, node.y + 16, 
                                fill=color, outline=outline, width=1.5)
            cv.create_text(node.x, node.y, text=node.val, font=("Consolas", 14, "bold"), fill=text_color)
            
        draw_tree(root)

class ParseTreeNode:
    def __init__(self, val, children=None):
        self.val = val
        self.children = children or []
    
    def width(self):
        if not self.children: 
            return max(50, len(str(self.val)) * 12)
        return sum(c.width() for c in self.children) + (len(self.children)-1)*15


if __name__ == "__main__":
    ScalrApp().mainloop()
