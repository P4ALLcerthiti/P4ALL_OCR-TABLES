#pragma once

#ifdef OCRTABS_EXPORT
#define OCRTABS_API __declspec(dllexport)
#elif  OCRTABS_IMPORT
#define OCRTABS_API __declspec(dllimport)
#else
#define OCRTABS_API 
#endif