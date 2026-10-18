// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: the line of its data neighbours (itmarkup); the TU keeps only data, so no instruction decides it; Research/runtime-compiler-ceiling.md
// LINK: after itmarkup; data EVNOVA 0x0057c140
/* regcomp.c -- the POSIX regular-expression compiler's character-class and collating-element tables (Henry Spencer's cclass.h and cname.h) and the two word-boundary bracket literals; its code is not in the exe, only this data. Notes: Research/notes/regcomp.md */

#include <stddef.h>

struct cclass {
    char *name;
    char *chars;
    char *multis;
};

struct cclass cclasses[] = {
    { "alnum", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "" },
    { "alpha", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", "" },
    { "blank", " \t", "" },
    { "cntrl", "\a\b\t\n\v\f\r\1\2\3\4\5\6\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\177", "" },
    { "digit", "0123456789", "" },
    { "graph", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", "" },
    { "lower", "abcdefghijklmnopqrstuvwxyz", "" },
    { "print", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ ", "" },
    { "punct", "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", "" },
    { "space", "\t\n\v\f\r ", "" },
    { "upper", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "" },
    { "xdigit", "0123456789ABCDEFabcdef", "" },
    { NULL, 0, "" }
};

struct cname {
    char *name;
    char code;
};

struct cname cnames[] = {
    { "NUL", '\0' },
    { "SOH", '\1' },
    { "STX", '\2' },
    { "ETX", '\3' },
    { "EOT", '\4' },
    { "ENQ", '\5' },
    { "ACK", '\6' },
    { "BEL", '\a' },
    { "alert", '\a' },
    { "BS", '\b' },
    { "backspace", '\b' },
    { "HT", '\t' },
    { "tab", '\t' },
    { "LF", '\n' },
    { "newline", '\n' },
    { "VT", '\v' },
    { "vertical-tab", '\v' },
    { "FF", '\f' },
    { "form-feed", '\f' },
    { "CR", '\r' },
    { "carriage-return", '\r' },
    { "SO", '\016' },
    { "SI", '\017' },
    { "DLE", '\020' },
    { "DC1", '\021' },
    { "DC2", '\022' },
    { "DC3", '\023' },
    { "DC4", '\024' },
    { "NAK", '\025' },
    { "SYN", '\026' },
    { "ETB", '\027' },
    { "CAN", '\030' },
    { "EM", '\031' },
    { "SUB", '\032' },
    { "ESC", '\033' },
    { "IS4", '\034' },
    { "FS", '\034' },
    { "IS3", '\035' },
    { "GS", '\035' },
    { "IS2", '\036' },
    { "RS", '\036' },
    { "IS1", '\037' },
    { "US", '\037' },
    { "space", ' ' },
    { "exclamation-mark", '!' },
    { "quotation-mark", '"' },
    { "number-sign", '#' },
    { "dollar-sign", '$' },
    { "percent-sign", '%' },
    { "ampersand", '&' },
    { "apostrophe", '\'' },
    { "left-parenthesis", '(' },
    { "right-parenthesis", ')' },
    { "asterisk", '*' },
    { "plus-sign", '+' },
    { "comma", ',' },
    { "hyphen", '-' },
    { "hyphen-minus", '-' },
    { "period", '.' },
    { "full-stop", '.' },
    { "slash", '/' },
    { "solidus", '/' },
    { "zero", '0' },
    { "one", '1' },
    { "two", '2' },
    { "three", '3' },
    { "four", '4' },
    { "five", '5' },
    { "six", '6' },
    { "seven", '7' },
    { "eight", '8' },
    { "nine", '9' },
    { "colon", ':' },
    { "semicolon", ';' },
    { "less-than-sign", '<' },
    { "equals-sign", '=' },
    { "greater-than-sign", '>' },
    { "question-mark", '?' },
    { "commercial-at", '@' },
    { "left-square-bracket", '[' },
    { "backslash", '\\' },
    { "reverse-solidus", '\\' },
    { "right-square-bracket", ']' },
    { "circumflex", '^' },
    { "circumflex-accent", '^' },
    { "underscore", '_' },
    { "low-line", '_' },
    { "grave-accent", '`' },
    { "left-brace", '{' },
    { "left-curly-bracket", '{' },
    { "vertical-line", '|' },
    { "right-brace", '}' },
    { "right-curly-bracket", '}' },
    { "tilde", '~' },
    { "DEL", '\177' },
    { NULL, 0 }
};

static char kWordStart[] = "[:<:]]";
static char kWordEnd[] = "[:>:]]";
