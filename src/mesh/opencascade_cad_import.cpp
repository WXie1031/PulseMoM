/**
 * OpenCascade CAD Import Integration
 * 
 * This module provides comprehensive CAD geometry import capabilities using OpenCascade.
 * Supports STEP, IGES, STL formats with geometry healing and repair functionality.
 * 
 * Features:
 * - Multi-format CAD import (STEP, IGES, STL)
 * - Geometry healing and repair
 * - Topology analysis and validation
 * - Surface extraction for mesh generation
 * - Multi-threading support
 * - Error handling and recovery
 */

#include "opencascade_cad_import.h"
#include <Standard_Version.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Message_ProgressIndicator.hxx>
#include <Message_ProgressSentry.hxx>

// Core OpenCascade headers
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>

// Import/Export headers
#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <StlAPI_Reader.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>
#include <TransferBRep_ShapeBinder.hxx>

// Geometry healing and repair
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Wireframe.hxx>
#include <ShapeFix_FixSmallFace.hxx>
#include <ShapeFix_Solid.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeFix_Edge.hxx>

// Analysis and validation
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Shape.hxx>
#include <BRepCheck_ListIteratorOfListOfStatus.hxx>

// Surface and curve analysis
#include <Geom_Surface.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Plane.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_BezierSurface.hxx>

// Topology exploration
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>

// Progress indication
#include <Message_ProgressIndicator.hxx>
#include <Message_ProgressSentry.hxx>

// Utilities
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <UnitsAPI.hxx>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>

// Thread safety
static std::mutex occt_mutex;

// Progress indicator implementation
class OCCTProgressIndicator : public Message_ProgressIndicator {
private:
    std::string operation_name;
    double last_percentage;
    
public:
    OCCTProgressIndicator(const std::string& name) 
        : operation_name(name), last_percentage(0.0) {}
    
    virtual bool Show(const bool force = false) {
        Standard_Real current = GetValue();
        Standard_Real min = GetMinValue();
        Standard_Real max = GetMaxValue();
        
        if (max > min) {
            double percentage = (current - min) / (max - min) * 100.0;
            if (force || std::abs(percentage - last_percentage) >= 5.0) {
                std::cout << "[OpenCascade] " << operation_name << ": " 
                         << std::fixed << std::setprecision(1) << percentage << "%" << std::endl;
                last_percentage = percentage;
            }
        }
        return true;
    }
};

// Error handler for OpenCascade exceptions
class OCCTErrorHandler {
public:
    static std::string getLastErrorMessage() {
        try {
            Handle(Standard_Failure) error = Standard_Failure::Caught();
            if (!error.IsNull()) {
                std::stringstream ss;
                ss << "OpenCascade Error: " << error->GetMessageString();
                return ss.str();
            }
        } catch (...) {
            return "Unknown OpenCascade error";
        }
        return "No error information available";
    }
    
    static std::string getShapeAnalysisMessage(const TopoDS_Shape& shape) {
        BRepCheck_Analyzer analyzer(shape, false);
        std::stringstream ss;
        
        if (!analyzer.IsValid()) {
            ss << "Shape validation failed: ";
            
            // Analyze specific issues
            for (analyzer.Init(); analyzer.More(); analyzer.Next()) {
                const TopoDS_Shape& current = analyzer.Current();
                Handle(BRepCheck_Result) result = analyzer.Result(current);
                
                if (!result.IsNull() && result->Status() != BRepCheck_NoError) {
                    BRepCheck_ListIteratorOfListOfStatus it(result->StatusOnShape());
                    for (; it.More(); it.Next()) {
                        ss << "Shape type " << current.ShapeType() << ": " << it.Value() << " ";
                    }
                }
            }
        } else {
            ss << "Shape is valid";
        }
        
        return ss.str();
    }
};

// Helper function to convert OpenCascade units to meters
static double getUnitScale(const std::string& unit_name) {
    if (unit_name == "MM") return 0.001;
    if (unit_name == "CM") return 0.01;
    if (unit_name == "M") return 1.0;
    if (unit_name == "INCH") return 0.0254;
    if (unit_name == "FOOT") return 0.3048;
    return 1.0; // Default to meters
}

// Forward declarations
static bool performGeometryHealing(TopoDS_Shape& shape, const opencascade_import_params_t* params);
static bool analyzeTopology(const TopoDS_Shape& shape, opencascade_geometry_info_t* info);
static bool extractSurfaces(const TopoDS_Shape& shape, std::vector<opencascade_surface_t>& surfaces);
static opencascade_surface_type_t classifySurface(const Handle(Geom_Surface)& surface);

// Main import function
bool opencascade_import_cad(const char* filename, opencascade_import_params_t* params, opencascade_geometry_t* geometry) {
    std::lock_guard<std::mutex> lock(occt_mutex);
    
    if (!filename || !geometry) {
        std::cerr << "[OpenCascade] Invalid parameters" << std::endl;
        return false;
    }
    
    try {
        OCCTErrorHandler error_handler;
        
        // Determine file format
        std::string filepath(filename);
        std::string extension;
        size_t dot_pos = filepath.find_last_of('.');
        if (dot_pos != std::string::npos) {
            extension = filepath.substr(dot_pos + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        }
        
        std::cout << "[OpenCascade] Importing file: " << filepath << " (format: " << extension << ")" << std::endl;
        
        TopoDS_Shape shape;
        bool import_success = false;
        
        // Import based on file format
        if (extension == "step" || extension == "stp") {
            import_success = opencascade_import_step(filename, params, &shape);
        } else if (extension == "iges" || extension == "igs") {
            import_success = opencascade_import_iges(filename, params, &shape);
        } else if (extension == "stl") {
            import_success = opencascade_import_stl(filename, params, &shape);
        } else {
            std::cerr << "[OpenCascade] Unsupported file format: " << extension << std::endl;
            return false;
        }
        
        if (!import_success) {
            std::cerr << "[OpenCascade] Import failed for file: " << filename << std::endl;
            return false;
        }
        
        std::cout << "[OpenCascade] Import successful, performing geometry analysis..." << std::endl;
        
        // Perform geometry healing if requested
        if (params && params->heal_geometry) {
            std::cout << "[OpenCascade] Performing geometry healing..." << std::endl;
            if (!performGeometryHealing(shape, params)) {
                std::cerr << "[OpenCascade] Geometry healing failed, continuing with original geometry" << std::endl;
            }
        }
        
        // Analyze topology
        opencascade_geometry_info_t info;
        if (!analyzeTopology(shape, &info)) {
            std::cerr << "[OpenCascade] Topology analysis failed" << std::endl;
            return false;
        }
        
        // Extract surfaces
        std::vector<opencascade_surface_t> surfaces;
        if (!extractSurfaces(shape, surfaces)) {
            std::cerr << "[OpenCascade] Surface extraction failed" << std::endl;
            return false;
        }
        
        // Fill geometry structure
        geometry->num_faces = info.num_faces;
        geometry->num_edges = info.num_edges;
        geometry->num_vertices = info.num_vertices;
        geometry->num_solids = info.num_solids;
        geometry->num_shells = info.num_shells;
        geometry->num_wires = info.num_wires;
        geometry->is_closed = info.is_closed;
        geometry->bounding_box[0] = info.bounding_box[0];
        geometry->bounding_box[1] = info.bounding_box[1];
        geometry->bounding_box[2] = info.bounding_box[2];
        geometry->bounding_box[3] = info.bounding_box[3];
        geometry->bounding_box[4] = info.bounding_box[4];
        geometry->bounding_box[5] = info.bounding_box[5];
        geometry->surface_area = info.surface_area;
        geometry->volume = info.volume;
        geometry->unit_scale = info.unit_scale;
        
        // Copy surfaces
        geometry->num_surfaces = surfaces.size();
        if (geometry->num_surfaces > 0) {
            geometry->surfaces = new opencascade_surface_t[geometry->num_surfaces];
            for (size_t i = 0; i < surfaces.size(); ++i) {
                geometry->surfaces[i] = surfaces[i];
            }
        } else {
            geometry->surfaces = nullptr;
        }
        
        // Store shape for later use (optional)
        geometry->occt_shape = new TopoDS_Shape(shape);
        
        std::cout << "[OpenCascade] Import completed successfully" << std::endl;
        std::cout << "[OpenCascade] Geometry summary:" << std::endl;
        std::cout << "  - Faces: " << geometry->num_faces << std::endl;
        std::cout << "  - Edges: " << geometry->num_edges << std::endl;
        std::cout << "  - Vertices: " << geometry->num_vertices << std::endl;
        std::cout << "  - Solids: " << geometry->num_solids << std::endl;
        std::cout << "  - Surface area: " << geometry->surface_area << " m²" << std::endl;
        std::cout << "  - Volume: " << geometry->volume << " m³" << std::endl;
        std::cout << "  - Surfaces: " << geometry->num_surfaces << std::endl;
        
        return true;
        
    } catch (const Standard_Failure& fail) {
        std::cerr << "[OpenCascade] Exception: " << fail.GetMessageString() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[OpenCascade] Standard exception: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[OpenCascade] Unknown exception during import" << std::endl;
        return false;
    }
}

bool opencascade_import_step(const char* filename, opencascade_import_params_t* params, void* shape) {
    try {
        OCCTProgressIndicator progress("STEP Import");
        
        STEPControl_Reader reader;
        reader.WS()->MapReader()->SetProgress(progress);
        
        // Read the file
        IFSelect_ReturnStatus status = reader.ReadFile(filename);
        if (status != IFSelect_RetDone) {
            std::cerr << "[OpenCascade] STEP read failed with status: " << status << std::endl;
            return false;
        }
        
        std::cout << "[OpenCascade] STEP file read successfully" << std::endl;
        
        // Transfer all roots
        bool failsonly = false;
        IFSelect_ReturnStatus transfer_status = reader.TransferRoots(failsonly);
        if (transfer_status != IFSelect_RetDone) {
            std::cerr << "[OpenCascade] STEP transfer failed with status: " << transfer_status << std::endl;
            return false;
        }
        
        // Get the resulting shape
        TopoDS_Shape& result_shape = *static_cast<TopoDS_Shape*>(shape);
        result_shape = reader.OneShape();
        
        if (result_shape.IsNull()) {
            std::cerr << "[OpenCascade] STEP import resulted in null shape" << std::endl;
            return false;
        }
        
        std::cout << "[OpenCascade] STEP import successful" << std::endl;
        return true;
        
    } catch (const Standard_Failure& fail) {
        std::cerr << "[OpenCascade] STEP import exception: " << fail.GetMessageString() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[OpenCascade] STEP import unknown exception" << std::endl;
        return false;
    }
}

bool opencascade_import_iges(const char* filename, opencascade_import_params_t* params, void* shape) {
    try {
        OCCTProgressIndicator progress("IGES Import");
        
        IGESControl_Reader reader;
        reader.WS()->MapReader()->SetProgress(progress);
        
        // Read the file
        IFSelect_ReturnStatus status = reader.ReadFile(filename);
        if (status != IFSelect_RetDone) {
            std::cerr << "[OpenCascade] IGES read failed with status: " << status << std::endl;
            return false;
        }
        
        std::cout << "[OpenCascade] IGES file read successfully" << std::endl;
        
        // Transfer all roots
        bool failsonly = false;
        IFSelect_ReturnStatus transfer_status = reader.TransferRoots(failsonly);
        if (transfer_status != IFSelect_RetDone) {
            std::cerr << "[OpenCascade] IGES transfer failed with status: " << transfer_status << std::endl;
            return false;
        }
        
        // Get the resulting shape
        TopoDS_Shape& result_shape = *static_cast<TopoDS_Shape*>(shape);
        result_shape = reader.OneShape();
        
        if (result_shape.IsNull()) {
            std::cerr << "[OpenCascade] IGES import resulted in null shape" << std::endl;
            return false;
        }
        
        std::cout << "[OpenCascade] IGES import successful" << std::endl;
        return true;
        
    } catch (const Standard_Failure& fail) {
        std::cerr << "[OpenCascade] IGES import exception: " << fail.GetMessageString() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[OpenCascade] IGES import unknown exception" << std::endl;
        return false;
    }
}

bool opencascade_import_stl(const char* filename, opencascade_import_params_t* params, void* shape) {
    try {
        OCCTProgressIndicator progress("STL Import");
        
        StlAPI_Reader reader;
        
        // Read the STL file
        TopoDS_Shape& result_shape = *static_cast<TopoDS_Shape*>(shape);
        
        if (!reader.Read(result_shape, filename)) {
            std::cerr << "[OpenCascade] STL read failed" << std::endl;
            return false;
        }
        
        if (result_shape.IsNull()) {
            std::cerr << "[OpenCascade] STL import resulted in null shape" << std::endl;
            return false;
        }
        
        std::cout << "[OpenCascade] STL import successful" << std::endl;
        return true;
        
    } catch (const Standard_Failure& fail) {
        std::cerr << "[OpenCascade] STL import exception: " << fail.GetMessageString() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[OpenCascade] STL import unknown exception" << std::endl;
        return false;
    }
}

static bool performGeometryHealing(TopoDS_Shape& shape, const opencascade_import_params_t* params) {
    try {
        std::cout << "[OpenCascade] Starting geometry healing process..." << std::endl;
        
        // Create shape fix tool
        Handle(ShapeFix_Shape) shape_fix = new ShapeFix_Shape();
        shape_fix->Init(shape);
        
        // Set healing parameters
        shape_fix->SetPrecision(params ? params->healing_precision : 1e-6);
        shape_fix->SetMaxTolerance(params ? params->max_tolerance : 1e-4);
        
        // Perform basic shape healing
        shape_fix->Perform();
        
        if (shape_fix->Status(ShapeExtend_DONE)) {
            std::cout << "[OpenCascade] Basic shape healing completed" << std::endl;
        }
        
        // Fix wireframe issues
        Handle(ShapeFix_Wireframe) wireframe_fix = new ShapeFix_Wireframe(shape);
        wireframe_fix->SetPrecision(params ? params->healing_precision : 1e-6);
        wireframe_fix->ModeDropSmallEdges() = Standard_True;
        wireframe_fix->FixSmallEdges();
        wireframe_fix->FixWireGaps();
        
        if (wireframe_fix->Status(ShapeExtend_DONE)) {
            std::cout << "[OpenCascade] Wireframe healing completed" << std::endl;
        }
        
        // Fix small faces
        Handle(ShapeFix_FixSmallFace) small_face_fix = new ShapeFix_FixSmallFace();
        small_face_fix->Init(shape);
        small_face_fix->Perform();
        
        if (small_face_fix->Status(ShapeExtend_DONE)) {
            std::cout << "[OpenCascade] Small face healing completed" << std::endl;
        }
        
        // Update the shape
        shape = shape_fix->Shape();
        
        std::cout << "[OpenCascade] Geometry healing completed successfully" << std::endl;
        return true;
        
    } catch (const Standard_Failure& fail) {
        std::cerr << "[OpenCascade] Geometry healing exception: " << fail.GetMessageString() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[OpenCascade] Geometry healing unknown exception" << std::endl;
        return false;
    }
}

static bool analyzeTopology(const TopoDS_Shape& shape, opencascade_geometry_info_t* info) {
    try {
        // Initialize counters
        info->num_faces = 0;
        info->num_edges = 0;
        info->num_vertices = 0;
        info->num_solids = 0;
        info->num_shells = 0;
        info->num_wires = 0;
        info->is_closed = false;
        info->surface_area = 0.0;
        info->volume = 0.0;
        info->unit_scale = 1.0;
        
        // Count topological entities
        TopTools_IndexedMapOfShape faces, edges, vertices, solids, shells, wires;
        
        TopExp::MapShapes(shape, TopAbs_FACE, faces);
        TopExp::MapShapes(shape, TopAbs_EDGE, edges);
        TopExp::MapShapes(shape, TopAbs_VERTEX, vertices);
        TopExp::MapShapes(shape, TopAbs_SOLID, solids);
        TopExp::MapShapes(shape, TopAbs_SHELL, shells);
        TopExp::MapShapes(shape, TopAbs_WIRE, wires);
        
        info->num_faces = faces.Extent();
        info->num_edges = edges.Extent();
        info->num_vertices = vertices.Extent();
        info->num_solids = solids.Extent();
        info->num_shells = shells.Extent();
        info->num_wires = wires.Extent();
        
        // Calculate bounding box
        Bnd_Box bbox;
        BRepBndLib::Add(shape, bbox);
        
        if (!bbox.IsVoid()) {
            Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
            bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            
            info->bounding_box[0] = xmin;
            info->bounding_box[1] = ymin;
            info->bounding_box[2] = zmin;
            info->bounding_box[3] = xmax;
            info->bounding_box[4] = ymax;
            info->bounding_box[5] = zmax;
        } else {
            for (int i = 0; i < 6; ++i) {
                info->bounding_box[i] = 0.0;
            }
        }
        
        // Calculate surface area
        GProp_GProps face_props;
        for (int i = 1; i <= faces.Extent(); ++i) {
            const TopoDS_Face& face = TopoDS::Face(faces(i));
            BRepGProp::SurfaceProperties(face, face_props);
            info->surface_area += face_props.Mass();
        }
        
        // Calculate volume (if solid)
        if (solids.Extent() > 0) {
            GProp_GProps solid_props;
            BRepGProp::VolumeProperties(shape, solid_props);
            info->volume = solid_props.Mass();
            info->is_closed = true; // Assume closed if we have solids
        }
        
        std::cout << "[OpenCascade] Topology analysis completed:" << std::endl;
        std::cout << "  - Faces: " << info->num_faces << std::endl;
        std::cout << "  - Edges: " << info->num_edges << std::endl;
        std::cout << "  - Vertices: " << info->num_vertices << std::endl;
        std::cout << "  - Solids: " << info->num_solids << std::endl;
        std::cout << "  - Surface area: " << info->surface_area << " m²" << std::endl;
        std::cout << "  - Volume: " << info->volume << " m³" << std::endl;
        
        return true;
        
    } catch (const Standard_Failure& fail) {
        std::cerr << "[OpenCascade] Topology analysis exception: " << fail.GetMessageString() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[OpenCascade] Topology analysis unknown exception" << std::endl;
        return false;
    }
}

static bool extractSurfaces(const TopoDS_Shape& shape, std::vector<opencascade_surface_t>& surfaces) {
    try {
        surfaces.clear();
        
        // Explore all faces in the shape
        TopExp_Explorer face_explorer(shape, TopAbs_FACE);
        int face_index = 0;
        
        while (face_explorer.More()) {
            const TopoDS_Face& face = TopoDS::Face(face_explorer.Current());
            
            // Get the underlying surface
            Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
            
            if (!surface.IsNull()) {
                opencascade_surface_t surf_info;
                surf_info.face_id = face_index++;
                surf_info.surface_type = classifySurface(surface);
                surf_info.is_planar = (surf_info.surface_type == OPENCASCADE_SURFACE_PLANE);
                surf_info.is_rational = surface->IsURational() || surface->IsVRational();
                
                // Get surface bounds
                Standard_Real umin, umax, vmin, vmax;
                surface->Bounds(umin, umax, vmin, vmax);
                surf_info.u_min = umin;
                surf_info.u_max = umax;
                surf_info.v_min = vmin;
                surf_info.v_max = vmax;
                
                // Calculate surface area
                GProp_GProps props;
                BRepGProp::SurfaceProperties(face, props);
                surf_info.area = props.Mass();
                
                // Get face normal at center
                try {
                    Standard_Real u = (umin + umax) / 2.0;
                    Standard_Real v = (vmin + vmax) / 2.0;
                    
                    gp_Pnt point;
                    gp_Vec normal;
                    surface->D1(u, v, point, normal);
                    
                    surf_info.center_point[0] = point.X();
                    surf_info.center_point[1] = point.Y();
                    surf_info.center_point[2] = point.Z();
                    
                    if (normal.Magnitude() > Precision::Confusion()) {
                        normal.Normalize();
                        surf_info.normal[0] = normal.X();
                        surf_info.normal[1] = normal.Y();
                        surf_info.normal[2] = normal.Z();
                    } else {
                        surf_info.normal[0] = 0.0;
                        surf_info.normal[1] = 0.0;
                        surf_info.normal[2] = 1.0;
                    }
                } catch (...) {
                    surf_info.center_point[0] = 0.0;
                    surf_info.center_point[1] = 0.0;
                    surf_info.center_point[2] = 0.0;
                    surf_info.normal[0] = 0.0;
                    surf_info.normal[1] = 0.0;
                    surf_info.normal[2] = 1.0;
                }
                
                // Store face reference
                surf_info.occt_face = new TopoDS_Face(face);
                
                surfaces.push_back(surf_info);
            }
            
            face_explorer.Next();
        }
        
        std::cout << "[OpenCascade] Extracted " << surfaces.size() << " surfaces" << std::endl;
        return true;
        
    } catch (const Standard_Failure& fail) {
        std::cerr << "[OpenCascade] Surface extraction exception: " << fail.GetMessageString() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[OpenCascade] Surface extraction unknown exception" << std::endl;
        return false;
    }
}

static opencascade_surface_type_t classifySurface(const Handle(Geom_Surface)& surface) {
    try {
        // Try to downcast to specific surface types
        Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(surface);
        if (!plane.IsNull()) return OPENCASCADE_SURFACE_PLANE;
        
        Handle(Geom_CylindricalSurface) cylinder = Handle(Geom_CylindricalSurface)::DownCast(surface);
        if (!cylinder.IsNull()) return OPENCASCADE_SURFACE_CYLINDER;
        
        Handle(Geom_ConicalSurface) cone = Handle(Geom_ConicalSurface)::DownCast(surface);
        if (!cone.IsNull()) return OPENCASCADE_SURFACE_CONE;
        
        Handle(Geom_SphericalSurface) sphere = Handle(Geom_SphericalSurface)::DownCast(surface);
        if (!sphere.IsNull()) return OPENCASCADE_SURFACE_SPHERE;
        
        Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast(surface);
        if (!torus.IsNull()) return OPENCASCADE_SURFACE_TORUS;
        
        Handle(Geom_BSplineSurface) bspline = Handle(Geom_BSplineSurface)::DownCast(surface);
        if (!bspline.IsNull()) return OPENCASCADE_SURFACE_BSPLINE;
        
        Handle(Geom_BezierSurface) bezier = Handle(Geom_BezierSurface)::DownCast(surface);
        if (!bezier.IsNull()) return OPENCASCADE_SURFACE_BEZIER;
        
        return OPENCASCADE_SURFACE_OTHER;
        
    } catch (...) {
        return OPENCASCADE_SURFACE_OTHER;
    }
}

// Cleanup function
void opencascade_free_geometry(opencascade_geometry_t* geometry) {
    if (!geometry) return;
    
    // Clean up surfaces
    if (geometry->surfaces) {
        for (int i = 0; i < geometry->num_surfaces; ++i) {
            if (geometry->surfaces[i].occt_face) {
                delete static_cast<TopoDS_Face*>(geometry->surfaces[i].occt_face);
            }
        }
        delete[] geometry->surfaces;
    }
    
    // Clean up shape
    if (geometry->occt_shape) {
        delete static_cast<TopoDS_Shape*>(geometry->occt_shape);
    }
    
    // Reset counters
    geometry->num_surfaces = 0;
    geometry->surfaces = nullptr;
    geometry->occt_shape = nullptr;
}

// Utility function to get version information
const char* opencascade_get_version() {
    return OCC_VERSION_STRING;
}

// Utility function to check if OpenCascade is available
bool opencascade_is_available() {
    try {
        // Test basic OpenCascade functionality
        TopoDS_Shape test_shape;
        return true;
    } catch (...) {
        return false;
    }
}