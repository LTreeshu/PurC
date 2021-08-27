/**
 * @file hvml.h
 * @author Xu Xiaohong
 * @date 2021/08/23
 * @brief The internal interfaces for hvml parser.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PURC_PRIVATE_HVML_H
#define PURC_PRIVATE_HVML_H

#include "purc-rwstream.h"
#include "private/stack.h"
#include "private/vcm.h"

enum hvml_state {
    HVML_DATA_STATE,
    HVML_RCDATA_STATE,
    HVML_RAWTEXT_STATE,
    HVML_PLAINTEXT_STATE,
    HVML_TAG_OPEN_STATE,
    HVML_END_TAG_OPEN_STATE,
    HVML_TAG_NAME_STATE,
    HVML_RCDATA_LESS_THAN_SIGN_STATE,
    HVML_RCDATA_END_TAG_OPEN_STATE,
    HVML_RCDATA_END_TAG_NAME_STATE,
    HVML_RAWTEXT_LESS_THAN_SIGN_STATE,
    HVML_RAWTEXT_END_TAG_OPEN_STATE,
    HVML_RAWTEXT_END_TAG_NAME_STATE,
    HVML_BEFORE_ATTRIBUTE_NAME_STATE,
    HVML_ATTRIBUTE_NAME_STATE,
    HVML_AFTER_ATTRIBUTE_NAME_STATE,
    HVML_BEFORE_ATTRIBUTE_VALUE_STATE,
    HVML_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE,
    HVML_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE,
    HVML_ATTRIBUTE_VALUE_UNQUOTED_STATE,
    HVML_AFTER_ATTRIBUTE_VALUE_QUOTED_STATE,
    HVML_SELF_CLOSING_START_TAG_STATE,
    HVML_BOGUS_COMMENT_STATE,
    HVML_MARKUP_DECLARATION_OPEN_STATE,
    HVML_COMMENT_START_STATE,
    HVML_COMMENT_START_DASH_STATE,
    HVML_COMMENT_STATE,
    HVML_COMMENT_LESS_THAN_SIGN_STATE,
    HVML_COMMENT_LESS_THAN_SIGN_BANG_STATE,
    HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE,
    HVML_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE,
    HVML_COMMENT_END_DASH_STATE,
    HVML_COMMENT_END_STATE,
    HVML_COMMENT_END_BANG_STATE,
    HVML_DOCTYPE_STATE,
    HVML_BEFORE_DOCTYPE_NAME_STATE,
    HVML_DOCTYPE_NAME_STATE,
    HVML_AFTER_DOCTYPE_NAME_STATE,
    HVML_AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE,
    HVML_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE,
    HVML_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE,
    HVML_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE,
    HVML_AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE,
    HVML_BETWEEN_DOCTYPE_PUBLIC_IDENTIFIER_AND_SYSTEM_INFORMATION_STATE,
    HVML_AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE,
    HVML_BEFORE_DOCTYPE_SYSTEM_INFORMATION_STATE,
    HVML_DOCTYPE_SYSTEM_INFORMATION_DOUBLE_QUOTED_STATE,
    HVML_DOCTYPE_SYSTEM_INFORMATION_SINGLE_QUOTED_STATE,
    HVML_AFTER_DOCTYPE_SYSTEM_INFORMATION_STATE,
    HVML_BOGUS_DOCTYPE_STATE,
    HVML_CDATA_SECTION_STATE,
    HVML_CDATA_SECTION_BRACKET_STATE,
    HVML_CDATA_SECTION_END_STATE,
    HVML_CHARACTER_REFERENCE_STATE,
    HVML_NAMED_CHARACTER_REFERENCE_STATE,
    HVML_AMBIGUOUS_AMPERSAND_STATE,
    HVML_NUMERIC_CHARACTER_REFERENCE_STATE,
    HVML_HEXADECIMAL_CHARACTER_REFERENCE_START_STATE,
    HVML_DECIMAL_CHARACTER_REFERENCE_START_STATE,
    HVML_HEXADECIMAL_CHARACTER_REFERENCE_STATE,
    HVML_DECIMAL_CHARACTER_REFERENCE_STATE,
    HVML_NUMERIC_CHARACTER_REFERENCE_END_STATE,
    HVML_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE,
    HVML_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE,
    HVML_EJSON_DATA_STATE,
    HVML_EJSON_FINISHED_STATE,
    HVML_EJSON_CONTROL_STATE,
    HVML_EJSON_LEFT_BRACE_STATE,
    HVML_EJSON_RIGHT_BRACE_STATE,
    HVML_EJSON_LEFT_BRACKET_STATE,
    HVML_EJSON_RIGHT_BRACKET_STATE,
    HVML_EJSON_LESS_THAN_SIGN_STATE,
    HVML_EJSON_GREATER_THAN_SIGN_STATE,
    HVML_EJSON_LEFT_PARENTHESIS_STATE,
    HVML_EJSON_RIGHT_PARENTHESIS_STATE,
    HVML_EJSON_DOLLAR_STATE,
    HVML_EJSON_AFTER_VALUE_STATE,
    HVML_EJSON_BEFORE_NAME_STATE,
    HVML_EJSON_AFTER_NAME_STATE,
    HVML_EJSON_NAME_UNQUOTED_STATE,
    HVML_EJSON_NAME_SINGLE_QUOTED_STATE,
    HVML_EJSON_NAME_DOUBLE_QUOTED_STATE,
    HVML_EJSON_VALUE_SINGLE_QUOTED_STATE,
    HVML_EJSON_VALUE_DOUBLE_QUOTED_STATE,
    HVML_EJSON_AFTER_VALUE_DOUBLE_QUOTED_STATE,
    HVML_EJSON_VALUE_TWO_DOUBLE_QUOTED_STATE,
    HVML_EJSON_VALUE_THREE_DOUBLE_QUOTED_STATE,
    HVML_EJSON_KEYWORD_STATE,
    HVML_EJSON_AFTER_KEYWORD_STATE,
    HVML_EJSON_BYTE_SEQUENCE_STATE,
    HVML_EJSON_AFTER_BYTE_SEQUENCE_STATE,
    HVML_EJSON_HEX_BYTE_SEQUENCE_STATE,
    HVML_EJSON_BINARY_BYTE_SEQUENCE_STATE,
    HVML_EJSON_BASE64_BYTE_SEQUENCE_STATE,
    HVML_EJSON_VALUE_NUMBER_STATE,
    HVML_EJSON_AFTER_VALUE_NUMBER_STATE,
    HVML_EJSON_VALUE_NUMBER_INTEGER_STATE,
    HVML_EJSON_VALUE_NUMBER_FRACTION_STATE,
    HVML_EJSON_VALUE_NUMBER_EXPONENT_STATE,
    HVML_EJSON_VALUE_NUMBER_EXPONENT_INTEGER_STATE,
    HVML_EJSON_VALUE_NUMBER_SUFFIX_INTEGER_STATE,
    HVML_EJSON_VALUE_NUMBER_INFINITY_STATE,
    HVML_EJSON_VALUE_NAN_STATE,
    HVML_EJSON_STRING_ESCAPE_STATE,
    HVML_EJSON_STRING_ESCAPE_FOUR_HEXADECIMAL_DIGITS_STATE,
    HVML_EJSON_JSONEE_VARIABLE_STATE,
    HVML_EJSON_JSONEE_FULL_STOP_SIGN_STATE,
    HVML_EJSON_JSONEE_KEYWORD_STATE,
    HVML_EJSON_JSONEE_STRING_STATE,
    HVML_EJSON_AFTER_JSONEE_STRING_STATE
};

enum hvml_token_type {
    HVML_TOKEN_DOCTYPE,
    HVML_TOKEN_START_TAG,
    HVML_TOKEN_END_TAG,
    HVML_TOKEN_COMMENT,
    HVML_TOKEN_CHARACTER,
    HVML_TOKEN_VCM_TREE,
    HVML_TOKEN_EOF
};

enum hvml_attribute_assignment {
    HVML_ATTRIBUTE_ASSIGNMENT,           // =
    HVML_ATTRIBUTE_ADDITION_ASSIGNMENT,  // +=
    HVML_ATTRIBUTE_SUBTRACTION_ASSIGNMENT, // -=
    HVML_ATTRIBUTE_REMAINDER_ASSIGNMENT,  // %=
    HVML_ATTRIBUTE_REPLACE_ASSIGNMENT,   // ~=
    HVML_ATTRIBUTE_HEAD_ASSIGNMENT,   // ^=
    HVML_ATTRIBUTE_TAIL_ASSIGNMENT,   // $=
};

struct temp_buffer;
struct pchvml_parser {
    enum hvml_state state;
    enum hvml_state return_state;
    uint32_t flags;
    uint32_t c_len;
    wchar_t wc;
    char c[8];
    size_t queue_size;
    struct temp_buffer* temp_buffer;
    bool need_reconsume;
};

struct pchvml_token_attribute {
    char* name;
    struct pcvcm_node* value;
    enum hvml_attribute_assignment assignment;
};

struct pchvml_token {
    enum hvml_token_type type;
    union {
        struct pchvml_token_attribute* attributes;
    };
};


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void pchvml_init_once (void);

struct pchvml_parser* pchvml_create(uint32_t flags, size_t queue_size);

void pchvml_reset(struct pchvml_parser* parser, uint32_t flags,
        size_t queue_size);

void pchvml_destroy(struct pchvml_parser* parser);


struct pchvml_token* pchvml_token_new (enum hvml_token_type type);
void pchvml_token_destroy (struct pchvml_token* token);

struct pchvml_token* pchvml_next_token (struct pchvml_parser* hvml,
                                          purc_rwstream_t rws);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_HVML_H */

