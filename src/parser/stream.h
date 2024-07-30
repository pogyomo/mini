#ifndef MINI_PARSER_STREAM_H_
#define MINI_PARSER_STREAM_H_

#include <memory>
#include <vector>

#include "../token.h"

namespace mini {

using TokenStreamState = size_t;

class TokenStream {
public:
    TokenStream(std::vector<std::unique_ptr<Token>>&& tokens)
        : offset_(0), tokens_(std::move(tokens)) {}
    explicit operator bool() { return offset_ < tokens_.size(); }
    inline void Advance() { offset_++; }
    inline const std::unique_ptr<Token>& CurrToken() const {
        return tokens_.at(offset_);
    }
    inline bool HasPrev() const { return offset_ > 0; }
    inline const std::unique_ptr<Token>& PrevToken() const {
        return tokens_.at(offset_ - 1);
    }
    inline const std::unique_ptr<Token>& Last() const {
        return tokens_.at(tokens_.size() - 1);
    }
    inline TokenStreamState State() const { return offset_; }
    inline void SetState(TokenStreamState state) { offset_ = state; }

private:
    size_t offset_;
    std::vector<std::unique_ptr<Token>> tokens_;
};

}  // namespace mini

#endif  // MINI_PARSER_STREAM_H_
