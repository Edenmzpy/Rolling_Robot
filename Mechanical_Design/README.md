# Mechanical design files

This directory contains the source and printable geometry for the Rolling Robot prototype.

## File formats

| Format | Use |
|---|---|
| `.SLDASM` | SolidWorks assemblies; use these to inspect component relationships |
| `.SLDPRT` | Editable SolidWorks part files |
| `.STL` | Mesh exports for slicing or viewing without SolidWorks |
| `.3MF` | Print-ready project files for selected cylindrical and ring components |

All design files are currently stored in [`Model/`](Model/). The Chinese filenames are the original working names from the prototype. They have not been translated or reorganized because SolidWorks assemblies may store path-based references to those exact names.

## Recommended workflow

1. Copy the complete `Model/` directory before making changes.
2. Open an assembly from the copied directory in SolidWorks.
3. If SolidWorks reports missing references, use **Find References** and point it to the same copied directory.
4. Export a fresh STL/3MF after editing instead of overwriting the provided printable file.

Dimensions, clearances, and material choices reflect the original research prototype. Verify them against your printer, actuator, corrugated tube, and fasteners before fabrication.
