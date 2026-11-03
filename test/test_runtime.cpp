#include "yt_js.h"
#include <iostream>

int main(int argc, char **argv) {
  try {
    if (argc != 3)
      throw std::runtime_error("expected Node and EJS directory arguments");
    const std::string node = argv[1];
    auto result = ytwe::run_js(node, R"JS(console.log('native process "quoted"');)JS");
    if (result.find(R"(native process "quoted")") == std::string::npos)
      throw std::runtime_error("stdin/stdout process roundtrip");
    auto directory = std::filesystem::path(argv[2]);
    auto script = ytwe::read_file(directory / "yt.solver.lib.js") +
        "\nObject.assign(globalThis, lib);\n" +
        ytwe::read_file(directory / "yt.solver.core.js") +
        R"JS(
const r = jsc({type:'preprocessed',
 preprocessed_player:'_result.n = s => s.split("").reverse().join(""); _result.sig = s => s.slice(1);',
 requests:[{type:'n',challenges:['abc']},{type:'sig',challenges:['xyz']}]});
if(r.responses[0].data.abc !== 'cba' || r.responses[1].data.xyz !== 'yz') throw Error('EJS result');
console.log('EJS roundtrip passed');
)JS";
    if (ytwe::run_js(node, script).find("EJS roundtrip passed") == std::string::npos)
      throw std::runtime_error("vendored EJS request/response");
    auto denied = ytwe::process({node, "--permission", "--no-warnings", "-e",
                                "require('fs').readFileSync(process.execPath)"},
                               nullptr, true, 5000);
    if (denied.code == 0)
      throw std::runtime_error("Node filesystem permission boundary");
    bool timed_out = false;
    try {
      ytwe::process({node, "-e", "setInterval(()=>{},1000)"}, nullptr, true, 100);
    } catch (const std::runtime_error &e) {
      timed_out = std::string(e.what()).find("exceeded process time/output limit") != std::string::npos;
    }
    if (!timed_out)
      throw std::runtime_error("child timeout did not terminate process");
    std::cout << "Native process, EJS, permissions and timeout checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
