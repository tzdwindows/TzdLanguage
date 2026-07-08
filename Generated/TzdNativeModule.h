#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <Eigen/Dense>

class TzdInterpreter;
struct TzdValue;

class TzdNativeModule {
public:
    static void init(TzdInterpreter* interp);

private:
    static void regInterpreterState(TzdInterpreter* interp);
    static void regMath(TzdInterpreter* interp);
    static void regMatrix(TzdInterpreter* interp);
    static void regSystem(TzdInterpreter* interp);
    static void regIO(TzdInterpreter* interp);
    static void regRuntime(TzdInterpreter* interp);
    static void regPlot(TzdInterpreter* interp);

    // ���?���
    static Eigen::MatrixXd toEigen(const TzdValue& arr);
    static TzdValue fromEigen(const Eigen::MatrixXd& mat);
    static std::string AnsiToUtf8(const std::string& str);
};