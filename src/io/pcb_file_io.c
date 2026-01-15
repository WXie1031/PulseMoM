/*********************************************************************
 * PCB文件输入输出接口实现
 * 支持Gerber、DXF、IPC-2581等主流PCB文件格式
 *********************************************************************/

#include "pcb_file_io.h"
#include <sys/stat.h>
#ifdef HAS_LIBXML2
#include <libxml/parser.h>
#include <libxml/tree.h>
#endif
#include <errno.h>
#include <time.h>
#include <stdarg.h>

// 全局错误状态
static int pcb_io_error_code = 0;
static char pcb_io_error_string[256] = {0};

// 设置错误信息
static void set_pcb_io_error(int code, const char* format, ...) {
    pcb_io_error_code = code;
    va_list args;
    va_start(args, format);
    vsnprintf(pcb_io_error_string, sizeof(pcb_io_error_string), format, args);
    va_end(args);
}

// 创建空的PCB设计
PCBDesign* create_empty_pcb_design(void) {
    PCBDesign* pcb = (PCBDesign*)calloc(1, sizeof(PCBDesign));
    if (!pcb) {
        set_pcb_io_error(1, "内存分配失败");
        return NULL;
    }
    
    // 设置默认值
    strcpy(pcb->design_name, "Untitled");
    strcpy(pcb->version, "1.0");
    strcpy(pcb->created_by, "PulseMoM");
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(pcb->creation_date, sizeof(pcb->creation_date), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // 材料属性默认值
    pcb->base_material_er = 4.4;          // FR4介电常数
    pcb->base_material_tan_delta = 0.02;  // FR4损耗角正切
    pcb->copper_conductivity = 5.8e7;     // 铜导电率 (S/m)
    
    return pcb;
}

// 销毁PCB设计
void destroy_pcb_design(PCBDesign* pcb) {
    if (!pcb) return;
    
    // 释放层信息
    if (pcb->layers) {
        free(pcb->layers);
    }
    
    // 释放几何图元
    if (pcb->primitives) {
        for (int i = 0; i < pcb->num_layers; i++) {
            PCBPrimitive* current = pcb->primitives[i];
            while (current) {
                PCBPrimitive* next = current->next;
                destroy_pcb_primitive(current);
                current = next;
            }
        }
        free(pcb->primitives);
    }
    
    // 释放每层图元数量数组
    if (pcb->num_primitives_per_layer) {
        free(pcb->num_primitives_per_layer);
    }
    
    // 释放网络名称
    if (pcb->net_names) {
        for (int i = 0; i < pcb->num_nets; i++) {
            if (pcb->net_names[i]) {
                free(pcb->net_names[i]);
            }
        }
        free(pcb->net_names);
    }
    
    // 释放元件信息
    if (pcb->component_names) {
        for (int i = 0; i < pcb->num_components; i++) {
            if (pcb->component_names[i]) {
                free(pcb->component_names[i]);
            }
        }
        free(pcb->component_names);
    }
    
    if (pcb->component_positions) {
        free(pcb->component_positions);
    }
    
    // 释放板框轮廓顶点
    if (pcb->outline.outline_vertices) {
        free(pcb->outline.outline_vertices);
    }
    
    free(pcb);
}

// 创建PCB图元
PCBPrimitive* create_pcb_primitive(PCBPrimitiveType type) {
    PCBPrimitive* primitive = NULL;
    
    switch (type) {
        case PCB_PRIM_LINE:
            primitive = (PCBPrimitive*)calloc(1, sizeof(PCBLine));
            break;
        case PCB_PRIM_ARC:
            primitive = (PCBPrimitive*)calloc(1, sizeof(PCBArc));
            break;
        case PCB_PRIM_CIRCLE:
            primitive = (PCBPrimitive*)calloc(1, sizeof(PCBCircle));
            break;
        case PCB_PRIM_RECTANGLE:
            primitive = (PCBPrimitive*)calloc(1, sizeof(PCBRectangle));
            break;
        case PCB_PRIM_POLYGON:
            primitive = (PCBPrimitive*)calloc(1, sizeof(PCBPolygon));
            break;
        case PCB_PRIM_PAD:
            primitive = (PCBPrimitive*)calloc(1, sizeof(PCBPad));
            break;
        case PCB_PRIM_VIA:
            primitive = (PCBPrimitive*)calloc(1, sizeof(PCBVia));
            break;
        case PCB_PRIM_BGA:
            primitive = (PCBPrimitive*)calloc(1, sizeof(PCBBGA));
            break;
        case PCB_PRIM_TEXT:
            primitive = (PCBPrimitive*)calloc(1, sizeof(PCBPrimitive));
            break;
        default:
            set_pcb_io_error(2, "不支持的图元类型: %d", type);
            return NULL;
    }
    
    if (!primitive) {
        set_pcb_io_error(3, "图元内存分配失败");
        return NULL;
    }
    
    primitive->type = type;
    primitive->id = -1; // 稍后分配ID
    primitive->line_width = 0.1; // 默认线宽 0.1mm
    primitive->layer_thickness = 0.035; // 默认铜厚度 35um
    primitive->layer_index = 0;
    strcpy(primitive->net_name, "");
    primitive->next = NULL;
    
    return primitive;
}

// 销毁PCB图元
void destroy_pcb_primitive(PCBPrimitive* primitive) {
    if (!primitive) return;
    
    // 特殊处理多边形
    if (primitive->type == PCB_PRIM_POLYGON) {
        PCBPolygon* polygon = (PCBPolygon*)primitive;
        if (polygon->vertices) {
            free(polygon->vertices);
        }
    }
    
    free(primitive);
}

// 添加图元到指定层
int add_primitive_to_layer(PCBDesign* pcb, int layer_index, PCBPrimitive* primitive) {
    if (!pcb || !primitive) return -1;
    if (layer_index < 0 || layer_index >= pcb->num_layers) {
        set_pcb_io_error(4, "层索引超出范围: %d", layer_index);
        return -1;
    }
    
    // 设置图元ID
    static int next_primitive_id = 1;
    primitive->id = next_primitive_id++;
    primitive->layer_index = layer_index;
    
    // 添加到链表头部
    primitive->next = pcb->primitives[layer_index];
    pcb->primitives[layer_index] = primitive;
    pcb->num_primitives_per_layer[layer_index]++;
    
    return primitive->id;
}

// 检测PCB文件格式
PCBFileFormat detect_pcb_file_format(const char* filename) {
    if (!filename) return PCB_FORMAT_GERBER_RS274X;
    
    // 获取文件扩展名
    const char* ext = strrchr(filename, '.');
    if (!ext) return PCB_FORMAT_GERBER_RS274X;
    
    // 转换为小写
    char ext_lower[32];
    strncpy(ext_lower, ext, sizeof(ext_lower) - 1);
    ext_lower[sizeof(ext_lower) - 1] = '\0';
    for (int i = 0; ext_lower[i]; i++) {
        ext_lower[i] = tolower(ext_lower[i]);
    }
    
    // 根据扩展名判断格式
    if (strstr(ext_lower, "gbr") || strstr(ext_lower, "ger")) {
        return PCB_FORMAT_GERBER_RS274X;
    } else if (strstr(ext_lower, "dxf")) {
        return PCB_FORMAT_DXF;
    } else if (strstr(ext_lower, "xml") || strstr(ext_lower, "ipc")) {
        return PCB_FORMAT_IPC2581;
    } else if (strstr(ext_lower, "kicad") || strstr(ext_lower, "kicad_pcb")) {
        return PCB_FORMAT_KICAD_PCB;
    } else if (strstr(ext_lower, "brd")) {
        return PCB_FORMAT_ALLEGRO; // Allegro 和 EAGLE 都使用 .brd
    } else if (strstr(ext_lower, "pcbdoc")) {
        return PCB_FORMAT_ALTIUM;
    }
    
    // 默认返回Gerber格式
    return PCB_FORMAT_GERBER_RS274X;
}

// 读取Gerber文件的基础实现
PCBDesign* read_gerber_rs274x(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        set_pcb_io_error(5, "无法打开文件: %s", filename);
        return NULL;
    }
    
    PCBDesign* pcb = create_empty_pcb_design();
    if (!pcb) {
        fclose(file);
        return NULL;
    }
    
    // 初始化Gerber解析状态
    GerberParserState state;
    memset(&state, 0, sizeof(state));
    state.file = file;
    state.absolute_coordinates = 1; // 默认绝对坐标
    state.unit_mode = 1; // 默认毫米
    state.scale_factor = 1.0;
    
    // 创建默认层（如未在文件中显式指明则暂采用双铜层）
    pcb->num_layers = 2;
    pcb->layers = (PCBLayerInfo*)calloc(pcb->num_layers, sizeof(PCBLayerInfo));
    pcb->primitives = (PCBPrimitive**)calloc(pcb->num_layers, sizeof(PCBPrimitive*));
    pcb->num_primitives_per_layer = (int*)calloc(pcb->num_layers, sizeof(int));
    
    // 设置默认层信息
    strcpy(pcb->layers[0].name, "Top Copper");
    pcb->layers[0].type = PCB_LAYER_COPPER;
    pcb->layers[0].thickness = 0.035; // 35um
    pcb->layers[0].copper_thickness = 35.0;
    pcb->layers[0].dielectric_constant = pcb->base_material_er;
    pcb->layers[0].loss_tangent = pcb->base_material_tan_delta;
    pcb->layers[0].elevation = 0.0;
    
    strcpy(pcb->layers[1].name, "Bottom Copper");
    pcb->layers[1].type = PCB_LAYER_COPPER;
    pcb->layers[1].thickness = 0.035;
    pcb->layers[1].copper_thickness = 35.0;
    pcb->layers[1].dielectric_constant = pcb->base_material_er;
    pcb->layers[1].loss_tangent = pcb->base_material_tan_delta;
    pcb->layers[1].elevation = -1.6; // 假设1.6mm板厚
    
    // 解析Gerber文件（增强版：记录D-codes与区域填充）
    int line_count = 0;
    int exposure_on = 0;
    int region_mode = 0;
    double current_aperture_diameter = 0.0;
    Point2D* region_points = NULL;
    int region_count = 0;
    int region_capacity = 0;
    while (fgets(state.current_line, sizeof(state.current_line), file)) {
        line_count++;
        
        // 去除行尾换行符
        char* newline = strchr(state.current_line, '\n');
        if (newline) *newline = '\0';
        
        // 跳过空行和注释
        if (state.current_line[0] == '\0' || state.current_line[0] == '*') continue;
        
        // 解析Gerber命令（简化实现）
        if (strstr(state.current_line, "G04")) {
            // 注释，忽略
            continue;
        } else if (strstr(state.current_line, "G01")) {
            // 线性插值模式
            continue;
        } else if (strstr(state.current_line, "G02")) {
            // 顺时针圆弧
            continue;
        } else if (strstr(state.current_line, "G03")) {
            // 逆时针圆弧
            continue;
        } else if (strstr(state.current_line, "G36")) {
            region_mode = 1;
            region_count = 0;
            region_capacity = 128;
            region_points = (Point2D*)realloc(region_points, region_capacity * sizeof(Point2D));
            continue;
        } else if (strstr(state.current_line, "G37")) {
            if (region_mode && region_count >= 3) {
                PCBPolygon* pg = (PCBPolygon*)create_pcb_primitive(PCB_PRIM_POLYGON);
                if (pg) {
                    pg->vertices = (Point2D*)malloc(region_count * sizeof(Point2D));
                    memcpy(pg->vertices, region_points, region_count * sizeof(Point2D));
                    pg->num_vertices = region_count;
                    pg->is_filled = 1;
                    add_primitive_to_layer(pcb, 0, (PCBPrimitive*)pg);
                }
            }
            region_mode = 0;
            continue;
        } else if (strstr(state.current_line, "D01")) {
            exposure_on = 1;
            continue;
        } else if (strstr(state.current_line, "D02")) {
            exposure_on = 0;
            continue;
        } else if (strstr(state.current_line, "D03")) {
            // 闪光曝光：创建圆形焊盘（简化）
            double x = state.current_line_x;
            double y = state.current_line_y;
            if (current_aperture_diameter > 0.0) {
                PCBCircle* pad = (PCBCircle*)create_pcb_primitive(PCB_PRIM_CIRCLE);
                if (pad) {
                    pad->center.x = x;
                    pad->center.y = y;
                    pad->radius = current_aperture_diameter * 0.5;
                    add_primitive_to_layer(pcb, 0, (PCBPrimitive*)pad);
                }
            }
            continue;
        } else if (state.current_line[0] == 'X' || state.current_line[0] == 'Y') {
            // 坐标数据
            double x = 0.0, y = 0.0;
            
            // 解析X坐标
            char* x_pos = strstr(state.current_line, "X");
            if (x_pos) {
                x = atof(x_pos + 1) * 1e-3; // 转换为mm（假设输入为微米）
            }
            
            // 解析Y坐标
            char* y_pos = strstr(state.current_line, "Y");
            if (y_pos) {
                y = atof(y_pos + 1) * 1e-3; // 转换为mm
            }
            
            if (region_mode) {
                if (region_count >= region_capacity) {
                    region_capacity *= 2;
                    region_points = (Point2D*)realloc(region_points, region_capacity * sizeof(Point2D));
                }
                region_points[region_count].x = x;
                region_points[region_count].y = y;
                region_count++;
            } else {
                if ((state.current_line_x != 0.0 || state.current_line_y != 0.0) && exposure_on) {
                    PCBLine* line = (PCBLine*)create_pcb_primitive(PCB_PRIM_LINE);
                    if (line) {
                        line->start.x = state.current_line_x;
                        line->start.y = state.current_line_y;
                        line->end.x = x;
                        line->end.y = y;
                        line->base.line_width = (state.current_line_width > 0.0) ? state.current_line_width : current_aperture_diameter;
                        add_primitive_to_layer(pcb, 0, (PCBPrimitive*)line);
                    }
                }
            }
            
            state.current_line_x = x;
            state.current_line_y = y;
        }
        
        // 解析光圈定义（支持圆形光圈）
        if (strstr(state.current_line, "ADD")) {
            char* size_pos = strstr(state.current_line, "C,");
            if (size_pos) {
                current_aperture_diameter = atof(size_pos + 2) * 1e-3;
                state.current_line_width = current_aperture_diameter;
            }
        }
    }
    
    fclose(file);
    if (region_points) { free(region_points); }
    
    // 依据已解析图元计算板框信息
    Point2D min_pt = {1e9, 1e9};
    Point2D max_pt = {-1e9, -1e9};
    calculate_pcb_bounds(pcb, &min_pt, &max_pt);
    if (min_pt.x < max_pt.x && min_pt.y < max_pt.y) {
        pcb->outline.bottom_left = min_pt;
        pcb->outline.top_right = max_pt;
    } else {
        // 兜底回退为100mm x 100mm
        pcb->outline.bottom_left.x = -50.0;
        pcb->outline.bottom_left.y = -50.0;
        pcb->outline.top_right.x = 50.0;
        pcb->outline.top_right.y = 50.0;
    }
    pcb->outline.board_thickness = 1.6; // 默认1.6mm，可在IPC-2581中覆盖
    
    printf("成功读取Gerber文件: %s\n", filename);
    printf("解析行数: %d\n", line_count);
    printf("层数: %d\n", pcb->num_layers);
    
    return pcb;
}

// 通用Gerber文件读取
PCBDesign* read_gerber_file(const char* filename) {
    return read_gerber_rs274x(filename); // 暂时只支持RS-274X
}

// 简化的DXF文件读取
PCBDesign* read_dxf_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        set_pcb_io_error(6, "无法打开DXF文件: %s", filename);
        return NULL;
    }
    
    PCBDesign* pcb = create_empty_pcb_design();
    if (!pcb) {
        fclose(file);
        return NULL;
    }
    
    // DXF解析状态
    DXFParserState state;
    memset(&state, 0, sizeof(state));
    state.file = file;
    
    // 简化的DXF解析
    char code_str[32], value_str[256];
    int current_code = 0;
    
    while (fgets(code_str, sizeof(code_str), file)) {
        // 读取代码
        current_code = atoi(code_str);
        
        // 读取对应的值
        if (!fgets(value_str, sizeof(value_str), file)) break;
        
        // 去除行尾换行符
        char* newline = strchr(value_str, '\n');
        if (newline) *newline = '\0';
        
        // 处理主要DXF实体
        if (current_code == 0 && strstr(value_str, "SECTION")) {
            // 开始新段
            continue;
        } else if (current_code == 0 && strstr(value_str, "ENDSEC")) {
            // 结束段
            continue;
        } else if (current_code == 0 && strstr(value_str, "LINE")) {
            // 开始线段实体
            state.current_entity_type = 1;
        } else if (current_code == 0 && strstr(value_str, "CIRCLE")) {
            // 开始圆形实体
            state.current_entity_type = 2;
        } else if (current_code == 0 && strstr(value_str, "ARC")) {
            // 开始圆弧实体
            state.current_entity_type = 3;
        } else if (current_code == 8) {
            // 图层名称
            state.current_layer = atoi(value_str);
        } else if (current_code == 10) {
            // X坐标
            state.current_position_x = atof(value_str);
        } else if (current_code == 20) {
            // Y坐标
            state.current_position_y = atof(value_str);
        } else if (current_code == 40) {
            // 半径或高度
            state.current_height = atof(value_str);
        }
    }
    
    fclose(file);
    
    // 创建默认层结构
    pcb->num_layers = 2;
    pcb->layers = (PCBLayerInfo*)calloc(pcb->num_layers, sizeof(PCBLayerInfo));
    pcb->primitives = (PCBPrimitive**)calloc(pcb->num_layers, sizeof(PCBPrimitive*));
    pcb->num_primitives_per_layer = (int*)calloc(pcb->num_layers, sizeof(int));
    
    printf("成功读取DXF文件: %s\n", filename);
    
    return pcb;
}

// IPC-2581文件读取（libxml2解析）
PCBDesign* read_ipc2581_file(const char* filename) {
    PCBDesign* pcb = create_empty_pcb_design();
    if (!pcb) return NULL;
    
    #ifdef HAS_LIBXML2
    xmlDocPtr doc = xmlReadFile(filename, NULL, 0);
    if (!doc) { set_pcb_io_error(7, "libxml2解析失败: %s", filename); return NULL; }
    xmlNodePtr root = xmlDocGetRootElement(doc);
    // 遍历获取层堆叠与材料属性
    int layers_cap = 16; int layers_count = 0;
    pcb->layers = (PCBLayerInfo*)calloc(layers_cap, sizeof(PCBLayerInfo));
    for (xmlNodePtr node = root; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, (const xmlChar*)"Layer") == 0) {
            xmlChar* name = xmlGetProp(node, (const xmlChar*)"name");
            xmlChar* type = xmlGetProp(node, (const xmlChar*)"type");
            xmlChar* thickness = xmlGetProp(node, (const xmlChar*)"thickness");
            if (layers_count >= layers_cap) { layers_cap *= 2; pcb->layers = (PCBLayerInfo*)realloc(pcb->layers, layers_cap*sizeof(PCBLayerInfo)); }
            PCBLayerInfo* L = &pcb->layers[layers_count++];
            strncpy(L->name, name ? (const char*)name : "Layer", sizeof(L->name)-1);
            L->type = PCB_LAYER_COPPER;
            if (type && strstr((const char*)type, "Dielectric")) L->type = PCB_LAYER_DIELECTRIC;
            L->thickness = thickness ? atof((const char*)thickness) : 0.1;
            L->dielectric_constant = pcb->base_material_er;
            L->loss_tangent = pcb->base_material_tan_delta;
            L->elevation = (layers_count==1) ? 0.0 : L->elevation - L->thickness;
            if (name) xmlFree(name); if (type) xmlFree(type); if (thickness) xmlFree(thickness);
        }
    }
    pcb->num_layers = layers_count;
    pcb->primitives = (PCBPrimitive**)calloc(pcb->num_layers, sizeof(PCBPrimitive*));
    pcb->num_primitives_per_layer = (int*)calloc(pcb->num_layers, sizeof(int));
    // xmlFreeDoc(doc);  // Requires libxml2, disabled for now
    #else
    set_pcb_io_error(9, "未启用libxml2，IPC-2581解析使用简化结构");
    pcb->num_layers = 4;
    pcb->layers = (PCBLayerInfo*)calloc(pcb->num_layers, sizeof(PCBLayerInfo));
    pcb->primitives = (PCBPrimitive**)calloc(pcb->num_layers, sizeof(PCBPrimitive*));
    pcb->num_primitives_per_layer = (int*)calloc(pcb->num_layers, sizeof(int));
    #endif
    return pcb;
}

// 通用PCB文件读取
PCBDesign* read_pcb_file(const char* filename, PCBFileFormat format) {
    if (!filename) {
        set_pcb_io_error(10, "文件名为空");
        return NULL;
    }
    
    switch (format) {
        case PCB_FORMAT_GERBER_RS274X:
        case PCB_FORMAT_GERBER_X2:
            return read_gerber_file(filename);
            
        case PCB_FORMAT_DXF:
            return read_dxf_file(filename);
            
        case PCB_FORMAT_IPC2581:
            return read_ipc2581_file(filename);
            
        default:
            set_pcb_io_error(11, "不支持的文件格式: %d", format);
            return NULL;
    }
}

// 验证PCB设计
int validate_pcb_design(const PCBDesign* pcb) {
    if (!pcb) {
        set_pcb_io_error(12, "PCB设计为空");
        return 0;
    }
    
    // 检查基本信息
    if (strlen(pcb->design_name) == 0) {
        set_pcb_io_error(13, "设计名称为空");
        return 0;
    }
    
    // 检查层信息
    if (pcb->num_layers <= 0) {
        set_pcb_io_error(14, "层数无效: %d", pcb->num_layers);
        return 0;
    }
    
    if (!pcb->layers) {
        set_pcb_io_error(15, "层信息为空");
        return 0;
    }
    
    // 检查几何图元
    if (!pcb->primitives) {
        set_pcb_io_error(16, "几何图元为空");
        return 0;
    }
    
    // 检查板框信息
    if (pcb->outline.bottom_left.x >= pcb->outline.top_right.x ||
        pcb->outline.bottom_left.y >= pcb->outline.top_right.y) {
        set_pcb_io_error(17, "板框坐标无效");
        return 0;
    }
    
    return 1;
}

// 打印PCB统计信息
void print_pcb_statistics(const PCBDesign* pcb) {
    if (!pcb) return;
    
    printf("\n=== PCB设计统计信息 ===\n");
    printf("设计名称: %s\n", pcb->design_name);
    printf("版本: %s\n", pcb->version);
    printf("创建工具: %s\n", pcb->created_by);
    printf("创建时间: %s\n", pcb->creation_date);
    printf("层数: %d\n", pcb->num_layers);
    printf("网络数: %d\n", pcb->num_nets);
    printf("元件数: %d\n", pcb->num_components);
    
    printf("\n层信息:\n");
    for (int i = 0; i < pcb->num_layers; i++) {
        printf("  层 %d: %s (类型:%d, 厚度:%.3fmm)\n", 
               i, pcb->layers[i].name, pcb->layers[i].type, pcb->layers[i].thickness);
    }
    
    printf("\n几何图元统计:\n");
    int total_primitives = 0;
    for (int i = 0; i < pcb->num_layers; i++) {
        int layer_primitives = pcb->num_primitives_per_layer[i];
        total_primitives += layer_primitives;
        printf("  层 %d: %d 个图元\n", i, layer_primitives);
    }
    printf("总计: %d 个图元\n", total_primitives);
    
    printf("\n板框信息:\n");
    printf("  尺寸: %.1f x %.1f mm\n", 
           pcb->outline.top_right.x - pcb->outline.bottom_left.x,
           pcb->outline.top_right.y - pcb->outline.bottom_left.y);
    printf("  厚度: %.2f mm\n", pcb->outline.board_thickness);
    
    printf("\n材料属性:\n");
    printf("  基材介电常数: %.2f\n", pcb->base_material_er);
    printf("  基材损耗角正切: %.4f\n", pcb->base_material_tan_delta);
    printf("  铜导电率: %.1e S/m\n", pcb->copper_conductivity);
    
    printf("========================\n");
}

// 计算PCB边界
void calculate_pcb_bounds(const PCBDesign* pcb, Point2D* min_point, Point2D* max_point) {
    if (!pcb || !min_point || !max_point) return;
    
    // 初始化为板框边界
    *min_point = pcb->outline.bottom_left;
    *max_point = pcb->outline.top_right;
    
    // 遍历所有图元更新边界
    for (int layer = 0; layer < pcb->num_layers; layer++) {
        PCBPrimitive* primitive = pcb->primitives[layer];
        while (primitive) {
            switch (primitive->type) {
                case PCB_PRIM_LINE: {
                    PCBLine* line = (PCBLine*)primitive;
                    min_point->x = fmin(min_point->x, fmin(line->start.x, line->end.x));
                    min_point->y = fmin(min_point->y, fmin(line->start.y, line->end.y));
                    max_point->x = fmax(max_point->x, fmax(line->start.x, line->end.x));
                    max_point->y = fmax(max_point->y, fmax(line->start.y, line->end.y));
                    break;
                }
                case PCB_PRIM_CIRCLE: {
                    PCBCircle* circle = (PCBCircle*)primitive;
                    double r = circle->radius;
                    min_point->x = fmin(min_point->x, circle->center.x - r);
                    min_point->y = fmin(min_point->y, circle->center.y - r);
                    max_point->x = fmax(max_point->x, circle->center.x + r);
                    max_point->y = fmax(max_point->y, circle->center.y + r);
                    break;
                }
                // 其他图元类型可以类似处理
                default:
                    break;
            }
            primitive = primitive->next;
        }
    }
}

// 按类型统计图元数量
int count_primitives_by_type(const PCBDesign* pcb, PCBPrimitiveType type) {
    if (!pcb) return 0;
    
    int count = 0;
    for (int layer = 0; layer < pcb->num_layers; layer++) {
        PCBPrimitive* primitive = pcb->primitives[layer];
        while (primitive) {
            if (primitive->type == type) {
                count++;
            }
            primitive = primitive->next;
        }
    }
    
    return count;
}

// 按网络统计图元数量
int count_primitives_by_net(const PCBDesign* pcb, const char* net_name) {
    if (!pcb || !net_name) return 0;
    
    int count = 0;
    for (int layer = 0; layer < pcb->num_layers; layer++) {
        PCBPrimitive* primitive = pcb->primitives[layer];
        while (primitive) {
            if (strcmp(primitive->net_name, net_name) == 0) {
                count++;
            }
            primitive = primitive->next;
        }
    }
    
    return count;
}

// 获取错误代码
int get_pcb_io_error_code(void) {
    return pcb_io_error_code;
}

// 获取错误描述
const char* get_pcb_io_error_string(void) {
    return pcb_io_error_string;
}

// 简化写入Gerber文件
int write_gerber_file(const PCBDesign* pcb, const char* filename, int layer_index) {
    if (!pcb || !filename) return -1;
    if (layer_index < 0 || layer_index >= pcb->num_layers) return -1;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        set_pcb_io_error(18, "无法创建Gerber文件: %s", filename);
        return -1;
    }
    
    // 写入Gerber头部
    fprintf(file, "G04 Generated by PulseMoM\n");
    fprintf(file, "G04 Layer: %s\n", pcb->layers[layer_index].name);
    fprintf(file, "%%FSLAX35Y35*%%\n"); // 格式声明 (%% escapes %)
    fprintf(file, "%%MOMM*%%\n"); // 单位：毫米 (%% escapes %)
    fprintf(file, "G01*%%\n"); // 线性插值 (%% escapes %)
    
    // 写入光圈定义（简化）
    fprintf(file, "%%ADD10C,0.100*%%\n"); // 默认0.1mm圆形光圈
    
    // 写入图元数据
    PCBPrimitive* primitive = pcb->primitives[layer_index];
    while (primitive) {
        switch (primitive->type) {
            case PCB_PRIM_LINE: {
                PCBLine* line = (PCBLine*)primitive;
                fprintf(file, "G01X%dY%dD02*\n", (int)(line->start.x * 1000), (int)(line->start.y * 1000));
                fprintf(file, "X%dY%dD01*\n", (int)(line->end.x * 1000), (int)(line->end.y * 1000));
                break;
            }
            case PCB_PRIM_CIRCLE: {
                PCBCircle* circle = (PCBCircle*)primitive;
                fprintf(file, "G03X%dY%dI0J0D03*\n", 
                        (int)(circle->center.x * 1000), (int)(circle->center.y * 1000));
                break;
            }
            default:
                break;
        }
        primitive = primitive->next;
    }
    
    // 写入结束
    fprintf(file, "M02*\n");
    
    fclose(file);
    printf("成功写入Gerber文件: %s\n", filename);
    
    return 0;
}

// 简化写入DXF文件
int write_dxf_file(const PCBDesign* pcb, const char* filename) {
    if (!pcb || !filename) return -1;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        set_pcb_io_error(19, "无法创建DXF文件: %s", filename);
        return -1;
    }
    
    // 写入DXF头部
    fprintf(file, "0\nSECTION\n2\nHEADER\n");
    fprintf(file, "9\n$ACADVER\n1\nAC1015\n");
    fprintf(file, "0\nENDSEC\n");
    
    // 写入表格段
    fprintf(file, "0\nSECTION\n2\nTABLES\n");
    fprintf(file, "0\nTABLE\n2\nLAYER\n70\n%d\n", pcb->num_layers);
    
    // 写入层定义
    for (int i = 0; i < pcb->num_layers; i++) {
        fprintf(file, "0\nLAYER\n2\n%s\n70\n0\n62\n%d\n6\nCONTINUOUS\n",
                pcb->layers[i].name, (i % 7) + 1); // 循环颜色
    }
    
    fprintf(file, "0\nENDTAB\n0\nENDSEC\n");
    
    // 写入实体段
    fprintf(file, "0\nSECTION\n2\nENTITIES\n");
    
    // 写入所有层图元
    for (int layer = 0; layer < pcb->num_layers; layer++) {
        PCBPrimitive* primitive = pcb->primitives[layer];
        while (primitive) {
            switch (primitive->type) {
                case PCB_PRIM_LINE: {
                    PCBLine* line = (PCBLine*)primitive;
                    fprintf(file, "0\nLINE\n8\n%s\n", pcb->layers[layer].name);
                    fprintf(file, "10\n%.6f\n20\n%.6f\n", line->start.x, line->start.y);
                    fprintf(file, "11\n%.6f\n21\n%.6f\n", line->end.x, line->end.y);
                    break;
                }
                case PCB_PRIM_CIRCLE: {
                    PCBCircle* circle = (PCBCircle*)primitive;
                    fprintf(file, "0\nCIRCLE\n8\n%s\n", pcb->layers[layer].name);
                    fprintf(file, "10\n%.6f\n20\n%.6f\n40\n%.6f\n", 
                            circle->center.x, circle->center.y, circle->radius);
                    break;
                }
                default:
                    break;
            }
            primitive = primitive->next;
        }
    }
    
    fprintf(file, "0\nENDSEC\n0\nEOF\n");
    
    fclose(file);
    printf("成功写入DXF文件: %s\n", filename);
    
    return 0;
}

// IPC-2581文件写入（框架）
int write_ipc2581_file(const PCBDesign* pcb, const char* filename) {
    if (!pcb || !filename) return -1;
    
    // IPC-2581需要完整的XML结构，这里提供框架
    set_pcb_io_error(20, "IPC-2581写入功能需要完整实现");
    return -1;
}
