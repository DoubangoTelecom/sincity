# Ragel generator
# For more information about Ragel: http://www.complang.org/ragel/

export OPTIONS="-C -L -T0"

ragel.exe $OPTIONS -o ./src/sc_parser_url.cc ./ragel/sc_parser_url.rl
