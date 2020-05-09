#pragma once

#include "../ast/node.h"



struct Context {
    ast::Ptr<ast::TranslationUnit> root;
};