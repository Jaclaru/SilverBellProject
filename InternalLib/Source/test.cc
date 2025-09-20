#include <iostream>

#include "CallbackSystem.hh"

#include "DirectedGraph.hh"
#include "DirectedGraphTraversal.hh"

using namespace SilverBell::TMP;


// 示例使用
void example1() {
    std::cout << "Example 1 called with no arguments" << std::endl;
}

void example2(int x) {
    std::cout << "Example 2 called with: " << x << std::endl;
}

void example3(int x, double y, const std::string& z) {
    std::cout << "Example 3 called with: " << x << ", " << y << ", " << z << std::endl;
}

void example4(const char* msg) {
    std::cout << "Example 4 called with: " << msg << std::endl;
}

// 自定义类型
struct Point {
    double x, y;

    friend std::ostream& operator<<(std::ostream& os, const Point& p) {
        return os << "(" << p.x << ", " << p.y << ")";
    }
};

// 自定义 variant 类型
using CustomVariant = std::variant<int, double, std::string, const char*, Point>;

void TestCallbackSystem()
{
    // 使用默认 variant 类型
    std::cout << "=== Using default variant type ===" << std::endl;
    GenericCallbackSystem<> defaultSystem;

    defaultSystem.SetParameter(0, 100);           // int
    defaultSystem.SetParameter(1, 3.14);          // double
    defaultSystem.SetParameter(2, "Hello");       // const char*
    defaultSystem.SetParameter(3, std::string("World"));  // std::string

    defaultSystem.Connect(example1);
    defaultSystem.Connect(example2);
    defaultSystem.Connect(example3);
    defaultSystem.Connect(example4);

    defaultSystem.Trigger();

    // 使用自定义 variant 类型
    std::cout << "\n=== Using custom variant type ===" << std::endl;
    GenericCallbackSystem<CustomVariant> customSystem;

    customSystem.SetParameter(0, 42);                    // int
    customSystem.SetParameter(1, 2.718);                 // double
    customSystem.SetParameter(2, "Custom");              // const char*
    customSystem.SetParameter(3, Point{ 1.0, 2.0 });       // Point

    customSystem.Connect(example1);
    customSystem.Connect(example2);
    //customSystem.Connect([](const Point& p) {
    //    std::cout << "Point callback called with: " << p << std::endl;
    //    });

    customSystem.Trigger();

    // 测试类型转换
    std::cout << "\n=== Testing type conversion ===" << std::endl;
    GenericCallbackSystem<> conversionSystem;

    conversionSystem.SetParameter(0, 42);        // int
    conversionSystem.SetParameter(1, "Test");    // const char*

    // 这个回调需要double参数，但参数是int类型（可以转换）
    conversionSystem.Connect([](double d) {
        std::cout << "Double value: " << d << std::endl;
        });

    //// 这个回调需要std::string参数，但参数是const char*类型（可以转换）
    //conversionSystem.Connect([](const std::string& s) {
    //    std::cout << "String value: " << s << std::endl;
    //    });

    conversionSystem.Trigger();

    // 测试参数访问
    std::cout << "\n=== Testing parameter access ===" << std::endl;
    GenericCallbackSystem<> accessSystem;

    accessSystem.SetParameter(0, 123);
    accessSystem.SetParameter(1, "Access Test");

    std::cout << "Parameter 0: " << accessSystem.GetParameter<int>(0) << std::endl;
    std::cout << "Parameter 1 is string: " << accessSystem.IsParameterType<std::string>(1) << std::endl;
}