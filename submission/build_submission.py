#!/usr/bin/env python3
"""
build_submission.py - builds ONE document for the COS202 lab submission.

Put this file in  <project>/submission/  together with:
    report.html
    uml_1_model.png, uml_2_controller.png, uml_3_helpers_exceptions.png, uml_4_gui.png
    screenshots/01_dashboard.png ... (see SCREENSHOTS below)

Run it from the project folder:
    python submission/build_submission.py        (or:  py submission/build_submission.py)

It writes  submission/COS202_SCAPMS_Submission.html  and opens it.
Then print that page to PDF (Ctrl+P > Save as PDF) from Microsoft Edge or Chrome.
Only the Python standard library is used.
"""

import base64
import datetime
import html
import pathlib
import sys
import webbrowser

# ----------------------------------------------------------------------------
# 1. EDIT THESE
# ----------------------------------------------------------------------------
STUDENT_NAME = "Almustapha Damilola Usman"
MATRIC_NUMBER = "2025/A/DSC/0063"
EMAIL = "A.USMAN9932@MIVA.EDU.NG"
DEPARTMENT = "DATA SCIENCE"
COURSE_TITLE = "COS202 Programming II - Lab Assessment Project"
PROJECT_TITLE = "Smart Clinic Appointment & Patient Management System (SCAPMS)"

# The screenshots the brief asks for: (file, caption, required?)
SCREENSHOTS = [
    ("screenshots/01_dashboard.png", "Main dashboard: live statistics, appointment reminders and the refresh clock", True),
    ("screenshots/02_patient_registration.png", "Patient registration: the form changes with the patient type; rows are coloured by category", True),
    ("screenshots/03_appointments.png", "Appointment management: status colours, status filter and the scheduling form", True),
    ("screenshots/04_search_sort.png", "Search and sort interface: algorithm and field selection, comparison counts", True),
    ("screenshots/05_waiting_queue.png", "Waiting queue: emergency patients first, live waiting times", True),
    ("screenshots/06_reports.png", "Reports: the four reports with the export button", True),
    ("screenshots/07_validation_error.png", "Input validation: a clear error dialog for invalid data", False),
    ("screenshots/08_waiting_alert.png", "Waiting-time alert raised by a timer", False),
]

UML_FIGURES = [
    ("uml_1_model.png", "Figure 1 - Model layer: inheritance, abstract class, interface, composition"),
    ("uml_2_controller.png", "Figure 2 - Controller layer: ClinicManager facade, queue, cache, undo stack"),
    ("uml_3_helpers_exceptions.png", "Figure 3 - Algorithm classes, helper classes and the exception hierarchy"),
    ("uml_4_gui.png", "Figure 4 - GUI layer: MainWindow and the ClinicPanel hierarchy"),
]

# ----------------------------------------------------------------------------
# 2. PATHS
# ----------------------------------------------------------------------------
SUB = pathlib.Path(__file__).resolve().parent
ROOT = SUB.parent
OUT = SUB / "COS202_SCAPMS_Submission.html"

warnings = []
stats = {"files": 0, "lines": 0}


def esc(text):
    return html.escape(text, quote=True)


def read_text(path):
    return path.read_text(encoding="utf-8", errors="replace")


def image_tag(path, caption, css_class=""):
    """An <img> with the picture embedded, so the HTML is one self-contained file."""
    if not path.exists():
        warnings.append("MISSING: " + str(path))
        return ('<p class="missing">MISSING IMAGE: %s<br>%s</p>' % (esc(path.name), esc(caption)))
    data = base64.b64encode(path.read_bytes()).decode("ascii")
    return ('<figure class="%s"><img src="data:image/png;base64,%s" alt="%s">'
            '<figcaption>%s</figcaption></figure>' % (css_class, data, esc(caption), esc(caption)))


def code_block(rel):
    """One source file as a heading plus a numbered listing."""
    path = ROOT / rel
    if not path.exists():
        warnings.append("MISSING: " + rel)
        return '<h4>%s</h4><p class="missing">FILE NOT FOUND</p>' % esc(rel)
    lines = read_text(path).splitlines()
    stats["files"] += 1
    stats["lines"] += len(lines)
    body = []
    for number, line in enumerate(lines, start=1):
        body.append('<span class="ln">%4d</span>  %s' % (number, esc(line)))
    return '<h4>%s <span class="count">(%d lines)</span></h4><pre>%s</pre>' % (
        esc(rel), len(lines), "\n".join(body))


def files_in(folder, pattern):
    directory = ROOT / folder
    if not directory.exists():
        warnings.append("MISSING FOLDER: " + folder)
        return []
    return ["%s/%s" % (folder, p.name) for p in sorted(directory.glob(pattern), key=lambda p: p.name.lower())]


def code_section(title, intro, rel_paths):
    parts = ['<div class="section"><h2>%s</h2><p class="intro">%s</p>' % (esc(title), esc(intro))]
    for rel in rel_paths:
        parts.append(code_block(rel))
    parts.append("</div>")
    return "\n".join(parts)


CSS = """
@page { size: A4; margin: 14mm 14mm 16mm 14mm; }
@page landscape { size: A4 landscape; margin: 10mm; }
* { box-sizing: border-box; }
body { font-family: Calibri, Arial, Helvetica, sans-serif; font-size: 10.5pt; color: #111; margin: 0; }
h1, h2, h3, h4 { page-break-after: avoid; }
h2 { font-size: 15pt; border-bottom: 2px solid #444; padding-bottom: 3px; margin: 0 0 8px 0; }
h4 { font-size: 10pt; margin: 14px 0 3px 0; font-family: Consolas, "Courier New", monospace; background: #e8ecf1; padding: 3px 6px; }
.count { font-weight: normal; color: #555; }
.section { page-break-before: always; }
.cover { text-align: center; padding-top: 55mm; }
.cover h1 { font-size: 26pt; margin: 0 0 6px 0; }
.cover .course { font-size: 15pt; margin-bottom: 26px; }
.cover .project { font-size: 17pt; margin-bottom: 40px; font-weight: bold; }
.cover table { margin: 0 auto; border-collapse: collapse; font-size: 12pt; }
.cover td { padding: 5px 16px; text-align: left; }
.cover td:first-child { font-weight: bold; text-align: right; }
.contents li { margin: 5px 0; }
.intro { color: #444; margin: 0 0 6px 0; }
pre { font-family: Consolas, "Courier New", monospace; font-size: 7.4pt; line-height: 1.25; background: #f7f8fa;
      border: 1px solid #d5d9e0; padding: 5px 6px; margin: 0 0 4px 0; white-space: pre-wrap; word-wrap: break-word; }
.ln { color: #9aa0aa; user-select: none; }
figure { margin: 0 0 14px 0; page-break-inside: avoid; text-align: center; }
figure img { max-width: 100%; max-height: 120mm; border: 1px solid #bbb; }
figcaption { font-size: 9.5pt; color: #333; margin-top: 4px; }
.uml { page: landscape; page-break-before: always; }
.uml img { max-width: 100%; max-height: 163mm; width: auto; border: none; }
.uml figcaption { font-size: 10pt; }
.legend { font-size: 8.5pt; color: #444; text-align: center; margin-top: 2px; }
.missing { color: #b00020; font-weight: bold; border: 2px dashed #b00020; padding: 8px; }
code { font-family: Consolas, "Courier New", monospace; font-size: 0.93em; background: #f0f2f5; padding: 0 2px; }
table { border-collapse: collapse; width: 100%; }
th, td { border: 1px solid #999; padding: 3px 6px; vertical-align: top; text-align: left; }
th { background: #e8ecf1; }
.report { font-size: 9.7pt; line-height: 1.28; }
.report h1 { font-size: 18pt; margin: 0; }
.report .sub { margin: 0 0 4px 0; color: #444; }
.report h2 { font-size: 11.5pt; margin: 8px 0 3px 0; border-bottom: 1px solid #888; padding-bottom: 1px; }
.report p { margin: 3px 0; text-align: justify; }
.report ul, .report ol { margin: 3px 0 3px 18px; padding: 0; }
.report li { margin: 1px 0; }
.report table { margin: 4px 0; font-size: 9pt; }
.report th, .report td { padding: 2px 5px; }
"""


def main():
    today = datetime.date.today().strftime("%d %B %Y")
    out = []

    # ---- cover -------------------------------------------------------------
    cover = f"""
<div class="cover">
  <h1>{esc(COURSE_TITLE)}</h1>
  <div class="course">Mid-semester lab assessment</div>
  <div class="project">{esc(PROJECT_TITLE)}</div>
  <table>
    <tr><td>Name:</td><td>{esc(STUDENT_NAME)}</td></tr>
    <tr><td>Matric number:</td><td>{esc(MATRIC_NUMBER)}</td></tr>
    <tr><td>Date:</td><td>{esc(today)}</td></tr>
    <tr><td>Technology:</td><td>C++17, Qt 6, CMake</td></tr>
  </table>
</div>
"""
    out.append(cover)

    # ---- contents + how to build -------------------------------------------
    out.append("""
<div class="section">
<h2>Contents</h2>
<ol class="contents">
  <li>Project report (2&ndash;3 pages)</li>
  <li>UML class diagrams (Figures 1&ndash;4)</li>
  <li>Screenshots of the application</li>
  <li>Source code: build file, header files, source files, GUI files, data files</li>
</ol>
<h2 style="margin-top:22px">How to build and run</h2>
<p>Requirements: MSYS2 UCRT64 with g++ (C++17), CMake, Ninja and Qt 6 (<code>mingw-w64-ucrt-x86_64-qt6-base</code>).</p>
<pre>cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
build\\scapms.exe</pre>
<p>In the application choose <b>File &gt; Load</b> and open <code>sample_clinic_data.csv</code> (12 patients, 19 appointments).
The build also produces <code>scapms_console.exe</code> (a console test of the core logic) and
<code>scapms_reminder_test.exe</code> (a test of the reminder logic).</p>
<h2 style="margin-top:22px">Project layout</h2>
<table>
<tr><th>Folder</th><th>Contents</th></tr>
<tr><td><code>model/</code></td><td>PatientRecord (abstract), GeneralPatient, EmergencyPatient, FollowUpPatient, Appointable (interface), Appointment, AppointmentBook, ClinicDatabase, PatientData, PatientFactory</td></tr>
<tr><td><code>controller/</code></td><td>ClinicManager (facade), Sorter, SearchEngine, WaitingQueue, RecentCache, UndoManager, ReportGenerator, ReminderService</td></tr>
<tr><td><code>utils/</code></td><td>Validator, IDGenerator, FileHandler, StringUtils, ClinicException</td></tr>
<tr><td><code>gui/</code></td><td>MainWindow, ClinicPanel (abstract), DashboardPanel, PatientPanel, AppointmentPanel, QueuePanel, SearchSortPanel, ReportPanel, PatientStyle</td></tr>
</table>
</div>
""")

    # ---- report (written in report.html) ------------------------------------
    report_path = SUB / "report.html"
    if report_path.exists():
        out.append('<div class="section">%s</div>' % read_text(report_path))
    else:
        warnings.append("MISSING: " + str(report_path))
        out.append('<div class="section"><p class="missing">report.html not found</p></div>')

    # ---- UML ----------------------------------------------------------------
    for filename, caption in UML_FIGURES:
        out.append('<div class="uml">%s<div class="legend">Hollow triangle = inheritance &nbsp;|&nbsp; '
                   'dashed line with hollow triangle = interface realisation &nbsp;|&nbsp; '
                   'filled diamond = composition (owns) &nbsp;|&nbsp; dashed arrow = dependency</div></div>'
                   % image_tag(SUB / filename, caption))

    # ---- screenshots ---------------------------------------------------------
    shots = ['<div class="section"><h2>Screenshots</h2>']
    number = 5
    for rel, caption, required in SCREENSHOTS:
        path = SUB / rel
        if not path.exists() and not required:
            continue                      # optional shot not taken: leave it out
        shots.append(image_tag(path, "Figure %d - %s" % (number, caption)))
        number += 1
    shots.append("</div>")
    out.append("\n".join(shots))

    # ---- source code ----------------------------------------------------------
    out.append(code_section("Source code - build file",
                            "CMake build description (compiles the core library, the console tests and the Qt application).",
                            ["CMakeLists.txt"]))

    headers = []
    for folder in ("model", "controller", "utils"):
        headers += files_in(folder, "*.h")
    out.append(code_section("Source code - header files",
                            "Declarations of the model, controller and utility classes.", headers))

    sources = []
    for folder in ("model", "controller", "utils"):
        sources += files_in(folder, "*.cpp")
    sources.append("main.cpp")
    out.append(code_section("Source code - source files",
                            "Implementations of the model, controller and utility classes, and the program entry point (main.cpp).",
                            sources))

    gui = files_in("gui", "*.h") + files_in("gui", "*.cpp")
    out.append(code_section("Source code - GUI files / resources",
                            "Qt windows and panels. (The GUI uses no image or resource files; everything is drawn by Qt.)", gui))

    out.append(code_section("Data files required to run the application",
                            "Sample data (12 patients, 19 appointments). Load it with File > Load.",
                            ["sample_clinic_data.csv"]))

    document = ("<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">"
                "<title>%s - %s</title><style>%s</style></head><body>%s</body></html>"
                % (esc(COURSE_TITLE), esc(PROJECT_TITLE), CSS, "\n".join(out)))
    OUT.write_text(document, encoding="utf-8")

    print("Wrote %s" % OUT)
    print("Source files included: %d (%d lines)" % (stats["files"], stats["lines"]))
    if warnings:
        print("\nPLEASE CHECK:")
        for w in warnings:
            print("  - " + w)
    else:
        print("Nothing missing.")
    print("\nNext: open the HTML file in Edge or Chrome, press Ctrl+P, choose 'Save as PDF',")
    print("paper size A4, scale 100%, tick 'Background graphics' and 'Headers and footers'.")

    if "--no-open" not in sys.argv:
        webbrowser.open(OUT.as_uri())


if __name__ == "__main__":
    main()
