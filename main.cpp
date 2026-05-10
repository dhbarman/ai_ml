#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "utils.h"

class SimpleTokenizer {
public:
    SimpleTokenizer() = default;

    bool BuildVocab(const std::string& filename);
    std::vector<int> Encode(const std::string& text);
	std::vector<std::string> Decode(const std::vector<int>& ids) const;
    void PrintVocab() const;

    const std::map<std::string, int>& GetVocab() const { return vocab_; }
	const std::map<int, std::string>& GetIdToTokenMap() const { return id_to_token_; }
    int Lookup(const std::string& token) const;
    uint32_t GetCharCount() const { return char_count_; }
    uint32_t GetLineCount() const { return line_count_; }

private:
    std::map<std::string, int> vocab_;
	std::map<int, std::string> id_to_token_;
    uint32_t char_count_ = 0;
    uint32_t line_count_ = 0;
	static const std::vector<std::string> special_context_tokens;
    static const std::regex token_pattern_;

    static std::ifstream OpenFile(const std::string& filename);
    static uint32_t CountUnicodeChars(const std::string& line);
    static std::vector<std::string> SplitByPattern(const std::string& line, const std::regex& pattern);
};

const std::vector<std::string> SimpleTokenizer::special_context_tokens = {
	"<|endoftext|>", "<|unk|>"
};
const std::regex SimpleTokenizer::token_pattern_(R"([,.:;?_!"()\']|--|\s)");

std::ifstream SimpleTokenizer::OpenFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        std::cerr << "Error opening file: " << filename << "\n";
    return file;
}

uint32_t SimpleTokenizer::CountUnicodeChars(const std::string& line) {
    uint32_t count = 0;
    for (size_t i = 0; i < line.size(); ++i) {
        unsigned char c = line[i];
        if      ((c & 0x80) == 0x00) { ++count;          }  // 1-byte
        else if ((c & 0xE0) == 0xC0) { ++count; i += 1;  }  // 2-byte
        else if ((c & 0xF0) == 0xE0) { ++count; i += 2;  }  // 3-byte
        else if ((c & 0xF8) == 0xF0) { ++count; i += 3;  }  // 4-byte
    }
    return count;
}

std::vector<std::string> SimpleTokenizer::SplitByPattern(
    const std::string& line, const std::regex& pattern)
{
    std::vector<std::string> result;
    std::sregex_token_iterator iter(line.begin(), line.end(), pattern, {-1, 0});
    for (; iter != std::sregex_token_iterator{}; ++iter) {
        std::string token = iter->str();
        if (!token.empty())
            result.push_back(std::move(token));
    }
    return result;
}

bool SimpleTokenizer::BuildVocab(const std::string& filename) {
    auto file = OpenFile(filename);
    if (!file.is_open()) return false;

    std::set<std::string> unique_words;
    std::string line;

    while (std::getline(file, line)) {
        char_count_ += CountUnicodeChars(line);
        ++line_count_;
        for (auto& token : SplitByPattern(line, token_pattern_)) {
            auto stripped = util::Strip(token);
            if (!stripped.empty())
                unique_words.insert(std::move(stripped));
        }
    }

    int id = 0;
    for (const std::string& word : unique_words) {
        vocab_[word] = id;
		id_to_token_[id++] = word;
	}
		

	for (const std::string& token : special_context_tokens) {
		vocab_[token] = id;
		id_to_token_[id++] = token;
	}

    return true;
}

std::vector<int> SimpleTokenizer::Encode(const std::string& text) {
    const auto words = SplitByPattern(text, token_pattern_);
    const int unk_id = Lookup("<|unk|>");
    std::vector<int> ids;
    for (const auto& word : words) {
        const auto stripped = util::Strip(word);
        if (stripped.empty()) continue;
        int id = Lookup(stripped);
        ids.push_back(id != -1 ? id : unk_id);
    }
    return ids;
}

std::vector<std::string> SimpleTokenizer::Decode(const std::vector<int>& ids) const {
	std::vector<std::string> tokens;
	for (int id : ids) {
		auto it = id_to_token_.find(id);
		if (it != id_to_token_.end())
			tokens.push_back(it->second);
		else
			tokens.push_back("<|unk|>");
	}
	return tokens;
}

void SimpleTokenizer::PrintVocab() const {
    for (const auto& entry : vocab_)
        std::cout << entry.first << ": " << entry.second << "\n";
    std::cout << "Vocab size: " << vocab_.size()
              << " | Chars: "   << char_count_
              << " | Lines: "   << line_count_  << "\n";
}

int SimpleTokenizer::Lookup(const std::string& token) const {
    auto it = vocab_.find(token);
    if (it == vocab_.end()) return -1;
    return it->second;
}

int main() {
    SimpleTokenizer tokenizer;
    if (!tokenizer.BuildVocab("data/the-verdict.txt"))
        return 1;
    tokenizer.PrintVocab();

    const std::string text1 = "Hello, do you like tea?";
    const std::string text2 = "In the sunlit terraces of the palce.";
    const std::string text   = text1 + " <|endoftext|> " + text2;
    const auto ids = tokenizer.Encode(text);
    for (int id : ids)
        std::cout << id << " ";
    std::cout << "\n"; 
	
	const auto decoded_tokens = tokenizer.Decode(ids);
	std::cout << "Decoded tokens:\n";
	for (const auto& token : decoded_tokens)
		std::cout << token << " ";
	std::cout << "\n";

	return 0;
}
