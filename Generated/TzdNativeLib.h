#ifndef TZD_NATIVE_LIB_H
#define TZD_NATIVE_LIB_H

#include <vector>
#include <string>

// 前向声明，避免循环引用
class TzdInterpreter;

class TzdNativeLib {
public:
    // 唯一的入口：为解释器注册所有原生函数
    static void registerAll(TzdInterpreter* interpreter);

private:
    // 内部按类别组织（可选，增加代码可读性）
    static void registerSystemFunctions(TzdInterpreter* interp);
    static void registerMathFunctions(TzdInterpreter* interp);
    static void registerIOFunctions(TzdInterpreter* interp);
    static void registerMatrixFunctions(TzdInterpreter* interp);
    static void registerAnalysisFunctions(TzdInterpreter* interp); // 导数、绘图等
};

#endif