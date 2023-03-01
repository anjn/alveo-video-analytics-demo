#pragma once

#define arg_begin(...)                      arg::parser args(argc,argv,##__VA_ARGS__)
#define arg_end                             args.check()

// for global variables
#define arg_g(var,defv,...)                      args.def(var,#var,#defv,##__VA_ARGS__)

#define arg_i(var,defv,...)   int var;           args.def(var,#var,#defv,##__VA_ARGS__)
#define arg_l(var,defv,...)   long int var;      args.def(var,#var,#defv,##__VA_ARGS__)
#define arg_ll(var,defv,...)  long long int var; args.def(var,#var,#defv,##__VA_ARGS__)
#define arg_d(var,defv,...)   double var;        args.def(var,#var,#defv,##__VA_ARGS__)
#define arg_b(var,defv,...)   bool var;          args.def(var,#var,#defv,##__VA_ARGS__)
#define arg_s(var,defv,...)   std::string var;   args.def(var,#var, defv,##__VA_ARGS__)

#define arg_iv(var,defv,...)  std::vector<int> var;         args.def(var,#var,#defv,##__VA_ARGS__)
#define arg_dv(var,defv,...)  std::vector<double> var;      args.def(var,#var,#defv,##__VA_ARGS__)
#define arg_sv(var,defv,...)  std::vector<std::string> var; args.def(var,#var, defv,##__VA_ARGS__)

#define xarg_i(var,defv) int var = defv;
#define xarg_l(var,defv) long int var = defv;
#define xarg_ll(var,defv) long long int var = defv;
#define xarg_d(var,defv) double var = defv;
#define xarg_b(var,defv) bool var = defv;
#define xarg_s(var,defv) string var = defv;
