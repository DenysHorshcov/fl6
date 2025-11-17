#include <clang-c/Index.h>
#include <iostream>
#include <vector>
#include <cstring>

bool does_lambda_capture_by_ref(CXCursor lambda_cursor) {
    CXTranslationUnit translation_unit = clang_Cursor_getTranslationUnit(lambda_cursor);
    if (!translation_unit) {
        return false; 
    }

    CXSourceRange source_range = clang_getCursorExtent(lambda_cursor);

    CXToken* token_array = nullptr;
    unsigned token_count = 0;
    clang_tokenize(translation_unit, source_range, &token_array, &token_count);
    
    if (!token_array || token_count == 0) {
        return false;
    }

    bool we_are_inside_capture_list = false;
    int current_bracket_depth = 0;  

    for (unsigned token_idx = 0; token_idx < token_count; ++token_idx) {
        CXToken current_token = token_array[token_idx];
        CXTokenKind token_type = clang_getTokenKind(current_token);
        CXString token_text = clang_getTokenSpelling(translation_unit, current_token);
        const char* text = clang_getCString(token_text);

        if (text != nullptr) { 
            if (strcmp(text, "[") == 0) {
                if (!we_are_inside_capture_list) {
                    we_are_inside_capture_list = true;
                    current_bracket_depth = 1;
                } else {
                    current_bracket_depth++; 
                }
            } 
            else if (strcmp(text, "]") == 0) {
                if (we_are_inside_capture_list) {
                    current_bracket_depth--;
                    if (current_bracket_depth == 0) {
                        we_are_inside_capture_list = false;
                        clang_disposeString(token_text);
                        break;  
                    }
                }
            } 
            else if (we_are_inside_capture_list) {
                if (token_type == CXToken_Punctuation && strcmp(text, "&") == 0) {
                    clang_disposeString(token_text);
                    clang_disposeTokens(translation_unit, token_array, token_count);
                    return true;  
                }
            }
        }

        clang_disposeString(token_text);
    }

    clang_disposeTokens(translation_unit, token_array, token_count);
    return false;
}

CXChildVisitResult ast_visitor(CXCursor cursor, CXCursor parent, CXClientData data) {
    (void)parent;

    CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    
    if (cursor_kind == CXCursor_LambdaExpr) {
        int* ref_capture_counter = static_cast<int*>(data);
        
        if (does_lambda_capture_by_ref(cursor)) {
            (*ref_capture_counter)++;
        }
    }

    return CXChildVisit_Recurse; 
}

int main(int argc, char** argv) {
    std::string input_filename;
    std::vector<const char*> compiler_args;

    if (argc < 2) {
        std::cout << "Enter path to .cpp file: ";
        std::getline(std::cin, input_filename);
        if (input_filename.empty()) {
            std::cerr << "No file provided.\n";
            return 1;
        }
        compiler_args.push_back("-std=c++17");
    } else {
        input_filename = argv[1];
        
        for (int arg_idx = 2; arg_idx < argc; ++arg_idx) {
            compiler_args.push_back(argv[arg_idx]);
        }
    }

    const char** clang_arguments = compiler_args.empty() ? nullptr : compiler_args.data();
    int argument_count = static_cast<int>(compiler_args.size());

    CXIndex clang_index = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
        clang_index,
        input_filename.c_str(),
        clang_arguments,
        argument_count,
        nullptr,  
        0,
        CXTranslationUnit_None
    );

    if (!tu) {
        std::cerr << "Failed to parse translation unit: " << input_filename << "\n";
        clang_disposeIndex(clang_index);
        return 1;
    }

    CXCursor root = clang_getTranslationUnitCursor(tu);

    int lambdas_with_ref_captures = 0;
    clang_visitChildren(root, ast_visitor, &lambdas_with_ref_captures);

    std::cout << "Number of lambdas with reference capture: "
              << lambdas_with_ref_captures << std::endl;


    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(clang_index);

    return 0;
}