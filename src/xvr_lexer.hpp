#ifndef XVR_LEXER_HPP
#define XVR_LEXER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "xvr_token_types.h"

namespace xvr {

using TokenType = Xvr_TokenType;

struct Token {
    TokenType type;
    std::string_view lexeme;
    int line;
    int column;
    
    Token(TokenType t, std::string_view l, int ln, int col)
        : type(t), lexeme(l), line(ln), column(col) {}
};

class Lexer {
public:
    explicit Lexer(std::string_view source);
    std::vector<Token> scanAll();
    std::optional<Token> nextToken();
    bool hasMore() const;
    void reset();
    
private:
    std::string_view source_;
    size_t current_{0};
    size_t start_{0};
    int line_{1};
    int column_{1};
    
    char peek() const;
    char peekAhead(size_t offset) const;
    char advance();
    bool match(char expected);
    void skipWhitespace();
    Token scanString();
    Token scanNumber();
    Token scanIdentifier();
    Token makeToken(TokenType type);
    void skipLine();
    
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
};

}  // namespace xvr

#endif  // XVR_LEXER_HPP