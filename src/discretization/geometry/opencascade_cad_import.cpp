/**
 * Minimal OpenCascade CAD import implementation.
 *
 * Uses OCCT to read STEP / IGES / STL files and fills the
 * opencascade_geometry_t struct defined in opencascade_cad_import.h.
 *
 * This version only depends on standard OCCT classes and the public
 * C API declared in the header, and is intended to compile against
 * your existing OpenCascade installation.
 */

#include "opencascade_cad_import.h"

#include <Standard_Failure.hxx>

#include <TopoDS_Shape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs.hxx>

#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <StlAPI_Reader.hxx>

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

using std::cerr;
using std::endl;
using std::string;

// Helper: import STEP file into a TopoDS_Shape
static bool import_step(const char* filename, TopoDS_Shape& out_shape) {
    try {
        STEPControl_Reader reader;
        IFSelect_ReturnStatus status = reader.ReadFile(filename);
        if (status != IFSelect_RetDone) {
            cerr << "[OpenCascade] STEP read failed, status=" << static_cast<int>(status) << endl;
            return false;
        }

        // Transfer all roots
        Standard_Integer nroots = reader.NbRootsForTransfer();
        for (Standard_Integer i = 1; i <= nroots; ++i) {
            reader.TransferRoot(i);
        }

        TopoDS_Shape shape = reader.OneShape();
        if (shape.IsNull()) {
            cerr << "[OpenCascade] STEP import produced null shape" << endl;
            return false;
        }
        out_shape = shape;
        return true;
    } catch (const Standard_Failure& fail) {
        cerr << "[OpenCascade] STEP import exception: " << fail.GetMessageString() << endl;
        return false;
    } catch (...) {
        cerr << "[OpenCascade] STEP import unknown exception" << endl;
        return false;
    }
}

// Helper: import IGES file into a TopoDS_Shape
static bool import_iges(const char* filename, TopoDS_Shape& out_shape) {
    try {
        IGESControl_Reader reader;
        IFSelect_ReturnStatus status = reader.ReadFile(filename);
        if (status != IFSelect_RetDone) {
            cerr << "[OpenCascade] IGES read failed, status=" << static_cast<int>(status) << endl;
            return false;
        }

        reader.TransferRoots();
        TopoDS_Shape shape = reader.OneShape();
        if (shape.IsNull()) {
            cerr << "[OpenCascade] IGES import produced null shape" << endl;
            return false;
        }
        out_shape = shape;
        return true;
    } catch (const Standard_Failure& fail) {
        cerr << "[OpenCascade] IGES import exception: " << fail.GetMessageString() << endl;
        return false;
    } catch (...) {
        cerr << "[OpenCascade] IGES import unknown exception" << endl;
        return false;
    }
}

// Helper: import STL file into a TopoDS_Shape
static bool import_stl(const char* filename, TopoDS_Shape& out_shape) {
    try {
        StlAPI_Reader reader;
        TopoDS_Shape shape;
        reader.Read(shape, filename);
        if (shape.IsNull()) {
            cerr << "[OpenCascade] STL import produced null shape" << endl;
            return false;
        }
        out_shape = shape;
        return true;
    } catch (const Standard_Failure& fail) {
        cerr << "[OpenCascade] STL import exception: " << fail.GetMessageString() << endl;
        return false;
    } catch (...) {
        cerr << "[OpenCascade] STL import unknown exception" << endl;
        return false;
    }
}

// Main C API used by cad_mesh_generation.c
bool opencascade_import_cad(const char* filename,
                            opencascade_import_params_t* params,
                            opencascade_geometry_t* geometry) {
    (void)params; // healing options not used in this minimal version

    if (!filename || !geometry) {
        return false;
    }

    std::memset(geometry, 0, sizeof(opencascade_geometry_t));

    // Detect extension
    string path(filename);
    string ext;
    std::size_t dot = path.find_last_of('.');
    if (dot != string::npos) {
        ext = path.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    TopoDS_Shape shape;
    bool ok = false;
    if (ext == "step" || ext == "stp") {
        ok = import_step(filename, shape);
    } else if (ext == "iges" || ext == "igs") {
        ok = import_iges(filename, shape);
    } else if (ext == "stl") {
        ok = import_stl(filename, shape);
    } else {
        cerr << "[OpenCascade] Unsupported CAD extension: " << ext << endl;
        return false;
    }

    if (!ok || shape.IsNull()) {
        return false;
    }

    // Compute bounding box
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    geometry->bounding_box[0] = xmin;
    geometry->bounding_box[1] = ymin;
    geometry->bounding_box[2] = zmin;
    geometry->bounding_box[3] = xmax;
    geometry->bounding_box[4] = ymax;
    geometry->bounding_box[5] = zmax;

    // Count faces, edges, vertices
    TopExp_Explorer exp;
    for (exp.Init(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        geometry->num_faces++;
    }
    for (exp.Init(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        geometry->num_edges++;
    }
    for (exp.Init(shape, TopAbs_VERTEX); exp.More(); exp.Next()) {
        geometry->num_vertices++;
    }

    // We don't distinguish solids/shells/wires in this minimal version
    geometry->num_solids = 0;
    geometry->num_shells = 0;
    geometry->num_wires  = 0;

    geometry->num_surfaces = 0;
    geometry->surfaces     = nullptr;

    // Unit scale: assume model units are meters for now
    geometry->unit_scale = 1.0;

    // Store OCCT shape pointer for possible later use
    geometry->occt_shape = new TopoDS_Shape(shape);

    return true;
}