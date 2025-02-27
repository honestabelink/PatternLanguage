#pragma once

#include <pl/core/ast/ast_node.hpp>

namespace pl::core::ast {

    class ASTNodeParameterPack : public ASTNode {
    public:
        explicit ASTNodeParameterPack(std::vector<Token::Literal> &&values) : m_values(std::move(values)) { }

        [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
            return std::unique_ptr<ASTNode>(new ASTNodeParameterPack(*this));
        }

        [[nodiscard]] const std::vector<Token::Literal> &getValues() const {
            return this->m_values;
        }

    private:
        std::vector<Token::Literal> m_values;
    };

}