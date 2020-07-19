#pragma once

#include "drawing_device.h"

struct DrawingML_Group : Drawing_Group {
    DrawingML_Group(int id, double width, double height, std::string name = "") :
        Drawing_Group(id, width, height, name) {}

    DrawingML_Group(int id, double x, double y, double width, double height, std::string name, std::vector<std::shared_ptr<Drawing_Geom>> &interior_objects) :
        Drawing_Group(id, x, y, width, height, name, interior_objects) {}

    virtual std::vector<XMLNode> xml() const;
};

struct DrawingML_Rect : Drawing_Rect {
    DrawingML_Rect(int id, double x0, double y0, double x1, double y1, Drawing_Attributes attributes) :
        Drawing_Rect(id, x0, y0, x1, y1, attributes) {}

    virtual std::vector<XMLNode> xml() const;
};

struct DrawingML_Line : Drawing_Line {
    DrawingML_Line(int id, double x1, double y1, double x2, double y2, Drawing_Attributes attributes) :
        Drawing_Line(id, x1, y1, x2, y2, attributes) {}

    virtual std::vector<XMLNode> xml() const;
};

struct DrawingML_Circle : Drawing_Circle {
    DrawingML_Circle(int id, double x, double y, double radius, Drawing_Attributes attributes) :
        Drawing_Circle(id, x, y, radius, attributes) {}

    virtual std::vector<XMLNode> xml() const;
};

struct DrawingML_Polyline : Drawing_Polyline {
    DrawingML_Polyline(int id, std::vector<std::pair<double, double>> &points, Drawing_Attributes attributes) :
        Drawing_Polyline(id, points, attributes) {}

    virtual std::vector<XMLNode> xml() const;
};

struct DrawingML_Polygon : Drawing_Polygon {
    DrawingML_Polygon(int id, std::vector<std::pair<double, double>> &points, Drawing_Attributes attributes) :
        Drawing_Polygon(id, points, attributes) {}

    virtual std::vector<XMLNode> xml() const;
};

struct DrawingML_Text : Drawing_Text {
    DrawingML_Text(int id, double x0, double y0, double x1, double y1, const std::string &text, Drawing_Alignment align, Drawing_Attributes attributes) :
        Drawing_Text(id, x0, y0, x1, y1, text, align, attributes) {}

    virtual std::vector<XMLNode> xml() const;
};

struct DrawingML_Context : Drawing_Context {
    DrawingML_Context() : Drawing_Context() {}
    virtual ~DrawingML_Context() {}

    virtual void initialise(double width, double height);
    virtual void group(int id, double width, double height, std::string name = "") {
        objects.emplace_back(std::make_shared<DrawingML_Group>(id, width, height, name));
    }
    virtual void group(int id, double x, double y, double width, double height, std::string name, std::vector<std::shared_ptr<Drawing_Geom>> &interior_objects) {
        objects.emplace_back(std::make_shared<DrawingML_Group>(id, x, y, width, height, name, interior_objects));
    }
    virtual void rect(int id, double x0, double y0, double x1, double y1, Drawing_Attributes attributes) {
        objects.emplace_back(std::make_shared<DrawingML_Rect>(id, x0, y0, x1, y1, attributes));
    }
    virtual void line(int id, double x1, double y1, double x2, double y2, Drawing_Attributes attributes) {
        objects.emplace_back(std::make_shared<DrawingML_Line>(id, x1, y1, x2, y2, attributes));
    }
    virtual void circle(int id, double x, double y, double radius, Drawing_Attributes attributes) {
        objects.emplace_back(std::make_shared<DrawingML_Circle>(id, x, y, radius, attributes));
    }
    virtual void polyline(int id, std::vector<std::pair<double, double>> &points, Drawing_Attributes attributes) {
        objects.emplace_back(std::make_shared<DrawingML_Polyline>(id, points, attributes));
    }
    virtual void polygon(int id, std::vector<std::pair<double, double>> &points, Drawing_Attributes attributes) {
        objects.emplace_back(std::make_shared<DrawingML_Polyline>(id, points, attributes));
    }
    virtual void text(int id, double x0, double y0, double x1, double y1, const std::string &text, Drawing_Alignment align, Drawing_Attributes attributes) {
        objects.emplace_back(std::make_shared<DrawingML_Text>(id, x0, y0, x1, y1, text, align, attributes));
    }

    virtual std::vector<std::pair<std::string, std::string>> container();

    std::string MLContainer_Content_Types();
    std::string MLContainer_Theme1(bool full_theme = false);
    std::string MLContainer_Relationships();
    std::string MLContainer_DrawingRelationships();
    std::string MLContainer_Drawing(const std::vector<std::shared_ptr<Drawing_Geom>> &objects);
};



