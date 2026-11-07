#ifndef TZD_EXPORT_H
#define TZD_EXPORT_H

#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(TZD_STATIC)
    #define TZD_API
  #elif defined(TZD_EXPORTS)
    #define TZD_API __declspec(dllexport)
  #else
    #define TZD_API __declspec(dllimport)
  #endif
#else
  #if defined(TZD_EXPORTS)
    #define TZD_API __attribute__((visibility("default")))
  #else
    #define TZD_API
  #endif
#endif

#endif /* TZD_EXPORT_H */
