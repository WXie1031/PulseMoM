/*********************************************************************
 * PCB文件输入输出接口头文件
 * 支持Gerber、DXF、IPC-2581等主流PCB文件格式
 *********************************************************************/

#ifndef PCB_FILE_IO_H
#define PCB_FILE_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

// PCB几何数据类型枚举
typedef enum {
    PCB_LAYER_COPPER,      // 铜层
    PCB_LAYER_DIELECTRIC,  // 介质层
    PCB_LAYER_SOLDERMASK,  // 阻焊层
    PCB_LAYER_SILKSCREEN,  // 丝印层
    PCB_LAYER_PASTE,       // 焊膏层
    PCB_LAYER_OUTLINE      // 轮廓层
} PCBLayerType;

// PCB几何图元类型
typedef enum {
    PCB_PRIM_LINE,         // 线段
    PCB_PRIM_ARC,          // 圆弧
    PCB_PRIM_CIRCLE,       // 圆
    PCB_PRIM_RECTANGLE,    // 矩形
    PCB_PRIM_POLYGON,      // 多边形
    PCB_PRIM_PAD,          // 焊盘
    PCB_PRIM_VIA,          // 过孔
    PCB_PRIM_BGA,          // BGA焊球
    PCB_PRIM_TEXT          // 文字
} PCBPrimitiveType;

// PCB文件格式枚举
typedef enum {
    PCB_FORMAT_GERBER_RS274X,    // Gerber RS-274X
    PCB_FORMAT_GERBER_X2,       // Gerber X2
    PCB_FORMAT_DXF,             // AutoCAD DXF
    PCB_FORMAT_IPC2581,         // IPC-2581
    PCB_FORMAT_ODBPP,           // ODB++
    PCB_FORMAT_KICAD_PCB,       // KiCad PCB
    PCB_FORMAT_ALLEGRO,         // Cadence Allegro
    PCB_FORMAT_ALTIUM,          // Altium Designer
    PCB_FORMAT_EAGLE            // Autodesk EAGLE
} PCBFileFormat;

// 2D点结构
typedef struct {
    double x, y;
} Point2D;

// 3D点结构
typedef struct {
    double x, y, z;
} Point3D;

// PCB几何图元基础结构
typedef struct PCBPrimitive {
    PCBPrimitiveType type;
    int id;
    char net_name[64];      // 网络名称
    double line_width;      // 线宽
    double layer_thickness; // 层厚度
    int layer_index;        // 层索引
    struct PCBPrimitive* next;
} PCBPrimitive;

// 线段结构
typedef struct {
    PCBPrimitive base;
    Point2D start, end;
} PCBLine;

// 圆弧结构
typedef struct {
    PCBPrimitive base;
    Point2D center;
    double radius;
    double start_angle;
    double end_angle;
} PCBArc;

// 圆形结构
typedef struct {
    PCBPrimitive base;
    Point2D center;
    double radius;
    int is_filled;      // 是否填充
} PCBCircle;

// 矩形结构
typedef struct {
    PCBPrimitive base;
    Point2D bottom_left;
    Point2D top_right;
    int is_filled;
} PCBRectangle;

// 多边形结构
typedef struct {
    PCBPrimitive base;
    Point2D* vertices;      // 顶点数组
    int num_vertices;
    int is_filled;
} PCBPolygon;

// 焊盘结构
typedef struct {
    PCBPrimitive base;
    Point2D position;
    double outer_diameter;  // 外径
    double inner_diameter;  // 内径
    double thermal_relief_gap; // 热隔离间隙
    int pad_shape;          // 焊盘形状：0=圆形, 1=矩形, 2=椭圆形
    double rotation_angle;  // 旋转角度
} PCBPad;

// 过孔结构
typedef struct {
    PCBPrimitive base;
    Point2D position;
    double diameter;        // 孔径
    int start_layer;        // 起始层
    int end_layer;          // 结束层
    int is_blind;           // 是否盲孔
    int is_buried;          // 是否埋孔
    int is_plated;          // 是否电镀
    double ring_thickness;  // 环形焊盘厚度
    double antipad_diameter; // 反焊盘孔径
    double stub_length;     // 盲孔stub长度
} PCBVia;

typedef struct {
    PCBPrimitive base;
    Point2D position;
    double ball_radius;
    int rows;
    int cols;
    double pitch;
    double rotation_angle;
    int layer_index;
} PCBBGA;

// PCB层信息
typedef struct {
    char name[64];          // 层名称
    PCBLayerType type;      // 层类型
    double thickness;       // 厚度 (mm)
    double copper_thickness; // 铜厚度 (um)
    double dielectric_constant; // 介电常数
    double loss_tangent;    // 损耗角正切
    int polarity;           // 极性：0=正片, 1=负片
    double elevation;       // 层高度 (mm)
} PCBLayerInfo;

// PCB设计规则
typedef struct {
    double min_line_width;        // 最小线宽
    double min_via_size;          // 最小过孔尺寸
    double min_via_drill;         // 最小钻孔尺寸
    double min_pad_size;          // 最小焊盘尺寸
    double min_clearance;         // 最小间距
    double min_copper_clearance;  // 最小铜间距
    double min_drill_to_edge;     // 钻孔到边缘最小距离
    double max_aspect_ratio;      // 最大纵横比
    int blind_buried_vias_allowed; // 允许盲埋孔
} PCBDesignRules;

// PCB板框信息
typedef struct {
    Point2D bottom_left;    // 左下角坐标
    Point2D top_right;      // 右上角坐标
    double board_thickness; // 板厚度
    Point2D* outline_vertices; // 轮廓顶点
    int num_outline_vertices;
} PCBOutline;

// PCB完整数据结构
typedef struct {
    // 基本信息
    char design_name[128];      // 设计名称
    char version[32];           // 版本
    char created_by[64];        // 创建工具
    char creation_date[64];     // 创建日期
    
    // 层信息
    PCBLayerInfo* layers;       // 层数组
    int num_layers;             // 层数
    
    // 几何图元
    PCBPrimitive** primitives;  // 每层图元链表数组
    int* num_primitives_per_layer; // 每层图元数量
    
    // 设计规则
    PCBDesignRules design_rules;
    
    // 板框信息
    PCBOutline outline;
    
    // 网络信息
    char** net_names;           // 网络名称数组
    int num_nets;               // 网络数量
    
    // 元器件信息
    char** component_names;     // 元件名称数组
    Point3D* component_positions; // 元件位置数组
    int num_components;         // 元件数量
    
    // 材料属性
    double base_material_er;    // 基材介电常数
    double base_material_tan_delta; // 基材损耗角正切
    double copper_conductivity; // 铜导电率
    
} PCBDesign;

// Gerber文件解析状态
typedef struct {
    FILE* file;
    char current_line[1024];
    int line_number;
    int current_layer;
    double current_aperture_size;
    int current_aperture_type;
    double current_line_width;
    double current_position_x;
    double current_position_y;
    double current_line_x;  // Added for compatibility
    double current_line_y;  // Added for compatibility
    int absolute_coordinates; // 1=绝对坐标, 0=相对坐标
    int unit_mode; // 0=英寸, 1=毫米
    double scale_factor;
} GerberParserState;

// DXF文件解析状态
typedef struct {
    FILE* file;
    char current_line[1024];
    double current_position_x;  // Added for compatibility
    double current_position_y;  // Added for compatibility
    int line_number;
    int current_layer;
    int in_entity_section;
    int in_table_section;
    int current_entity_type;
    double current_height;
} DXFParserState;

// IPC-2581文件解析状态
typedef struct {
    FILE* file;
    char* buffer;
    size_t buffer_size;
    int current_layer;
    int current_net;
    void* current_node;  // xmlNodePtr - requires libxml2, using void* for now
    void* doc;           // xmlDocPtr - requires libxml2, using void* for now
} IPC2581ParserState;

// PCB文件读取函数声明
PCBDesign* create_empty_pcb_design(void);
void destroy_pcb_design(PCBDesign* pcb);

// 文件格式检测
PCBFileFormat detect_pcb_file_format(const char* filename);

// Gerber文件读取
PCBDesign* read_gerber_file(const char* filename);
PCBDesign* read_gerber_rs274x(const char* filename);
PCBDesign* read_gerber_x2(const char* filename);

// DXF文件读取
PCBDesign* read_dxf_file(const char* filename);

// IPC-2581文件读取
PCBDesign* read_ipc2581_file(const char* filename);

// 通用PCB文件读取
PCBDesign* read_pcb_file(const char* filename, PCBFileFormat format);

// PCB数据验证
int validate_pcb_design(const PCBDesign* pcb);
void print_pcb_statistics(const PCBDesign* pcb);

// PCB几何处理
void calculate_pcb_bounds(const PCBDesign* pcb, Point2D* min_point, Point2D* max_point);
int count_primitives_by_type(const PCBDesign* pcb, PCBPrimitiveType type);
int count_primitives_by_net(const PCBDesign* pcb, const char* net_name);

// PCB导出功能
int write_gerber_file(const PCBDesign* pcb, const char* filename, int layer_index);
int write_dxf_file(const PCBDesign* pcb, const char* filename);
int write_ipc2581_file(const PCBDesign* pcb, const char* filename);

// 内存管理辅助函数
PCBPrimitive* create_pcb_primitive(PCBPrimitiveType type);
void destroy_pcb_primitive(PCBPrimitive* primitive);
int add_primitive_to_layer(PCBDesign* pcb, int layer_index, PCBPrimitive* primitive);

// 错误处理
const char* get_pcb_io_error_string(void);
int get_pcb_io_error_code(void);

#endif // PCB_FILE_IO_H
