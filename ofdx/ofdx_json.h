/*
	ofdx_json
	mperron (2024)

	Basic JSON library (Javascript Object Notation)
	https://www.json.org/json-en.html


	TODO Support for floating point numbers
*/

#ifndef OFDX_JSON_H
#define OFDX_JSON_H

#include <unordered_map>
#include <string>
#include <sstream>
#include <memory>
#include <vector>

class Json {
	// Increment i to the next non-whitespace character
	// (specifically space, carriage return, line feed, horizontal tab)
	// Return false if whitespace extends to the end of the string.
	static bool skipWhitespace(std::string const& blob, int &i){
		for(
			char c = blob[i];
			((c == ' ') || (c == '\r') || (c == '\n') || (c == '\t'));
			c = blob[++ i]
		){
			// Hit the end of the string without finding a value.
			if(i >= blob.size())
				return false;
		}

		return true;
	}

public:
	// Abstract value type, should be implemented by actual data types.
	struct Value {
		enum Type {
			JSON_VALUE_STRING,
			JSON_VALUE_OBJECT,
			JSON_VALUE_ARRAY,
			JSON_VALUE_BOOL,
			JSON_VALUE_NULL,
			JSON_VALUE_INT
		} m_type;

		Value(Type t) :
			m_type(t)
		{}

		Type const type() const {
			return m_type;
		}

		std::string str() const;
	};
	typedef std::shared_ptr<Json::Value> Value_p;

	// Type of the root node map (key to value).
	typedef std::unordered_map<std::string, Value_p> root_t;
	typedef std::shared_ptr<root_t> root_p;

	// A potentially empty list of values.
	struct Array : public Value {
		std::vector<Json::Value_p> m_data;

		Array() :
			Value(JSON_VALUE_ARRAY)
		{}

		Json::Value_p get(int index) const {
			return m_data[index];
		}

		size_t size() const {
			return m_data.size();
		}

		void append(Json::Value_p el){
			m_data.push_back(el);
		}
	};
	typedef std::shared_ptr<Json::Array> Array_p;

	// Integer numbers.
	struct Int : public Value {
		int64_t const m_data;

		Int(int64_t const value) :
			Value(JSON_VALUE_INT),
			m_data(value)
		{}

		int64_t get() const {
			return m_data;
		}
	};
	typedef std::shared_ptr<Json::Int> Int_p;

	// Boolean values, literal true/false
	struct Bool : public Value {
		bool const m_data;

		Bool(bool const value) :
			Value(JSON_VALUE_BOOL),
			m_data(value)
		{}

		bool get() const {
			return m_data;
		}
	};
	typedef std::shared_ptr<Json::Bool> Bool_p;

	struct Null : public Value {
		Null() :
			Value(JSON_VALUE_NULL)
		{}
	};
	typedef std::shared_ptr<Json::Null> Null_p;

	// Double-quote enclosed string values
	struct String : public Value {
		std::string const m_data;

		String(std::string const value) :
			Value(JSON_VALUE_STRING),
			m_data(value)
		{}

		std::string const get() const {
			return m_data;
		}
	};
	typedef std::shared_ptr<Json::String> String_p;

	// Everything should be parented by a Json::Object.
	struct Object : public Value {
		root_p m_data;

		Object(root_p obj) :
			Value(JSON_VALUE_OBJECT),
			m_data(obj)
		{}

		root_p get() const {
			return m_data;
		}

		Json::Value_p getValue(std::string const& key) const {
			return (*m_data)[key];
		}

		// Usage example: this->get<Json::String>("body");
		template<typename T>
		std::shared_ptr<T> get(std::string const& key) const {
			return std::static_pointer_cast<T>(getValue(key));
		}

		// Produce a JSON string for transport.
		std::string str() const {
			std::stringstream ss;
			bool first = true;

			ss << "{";

			for(auto const& kv : *m_data){
				// Write a comma if this object has two or more children.
				if(first){
					first = false;
				} else {
					ss << ",";
				}

				// Write out key:value
				ss << "\"" << kv.first << "\":" << kv.second->str();
			}

			ss << "}";
			return ss.str();
		}
	};
	typedef std::shared_ptr<Json::Object> Object_p;

private:

	// Get a value (no key) starting from the current position in the blob string.
	static Json::Value_p parseValue(std::string const& blob, int &i){
		Json::Value_p value_box;

		if(!skipWhitespace(blob, i))
			return nullptr;

		switch(blob[i]){
			// Parse double-quoted value.
			case '"':
				{
					std::string const valuesource(blob.substr(i + 1));
					size_t n = std::string::npos;

					for(int ii = 0; ii < valuesource.size(); ++ ii){
						// Found matching double-quote.
						if(valuesource[ii] == '"'){
							n = ii;
							break;
						}

						// Skip the next character if we hit a backslash.
						if(valuesource[ii] == '\\'){
							++ ii;
							continue;
						}
					}

					if(n != std::string::npos){
						i += n + 1;

						// String ends sooner than expected!
						if(i >= blob.size())
							return nullptr;

						return std::make_shared<Json::String>(valuesource.substr(0, n));
					}
				}
				break;

			// Nested object, recurse to populate.
			case '{':
				return parseObject(blob, ++ i);

			// Booleans
			case 't':
				if(blob.substr(i).find("true") == 0){
					i += 3; // length of "true" - 1
					return std::make_shared<Json::Bool>(true);
				}
				break;
			case 'f':
				if(blob.substr(i).find("false") == 0){
					i += 4; // length of "false" - 1
					return std::make_shared<Json::Bool>(false);
				}
				break;

			case 'n':
				if(blob.substr(i).find("null") == 0){
					i += 3; // length of "null" - 1
					return std::make_shared<Json::Null>();
				}
				break;

			// Numbers
			case '0': case '1': case '2':
			case '3': case '4': case '5':
			case '6': case '7': case '8':
			case '9': case '-': case '+':
				// Parse numerical value
				{
					// FIXME - add support for floats
					bool isNegative = false;

					switch(blob[i]){
						case '-': isNegative = true;
						case '+': ++ i;
					}

					size_t const n = blob.substr(i).find_first_not_of("0123456789");

					if(n == std::string::npos)
						return nullptr;

					int64_t vll = std::stoll(blob.substr(i, n));

					if(isNegative)
						vll = -vll;

					return std::make_shared<Json::Int>(vll);
				}
				break;

			// Array
			case '[':
				return parseArray(blob, i);
		}

		// Unsupported/unknown/invalid
		return nullptr;
	}

	// Return an array built from the JSON string provided.
	static Json::Array_p parseArray(std::string const& blob, int &i){
		Json::Array_p root = std::make_shared<Json::Array>();

		for(++ i; i < blob.size(); ++ i){
			Json::Value_p value_box = parseValue(blob, i);

			if(value_box){
				root->append(value_box);
				++ i;
			}

			if(!skipWhitespace(blob, i))
				return nullptr;

			switch(blob[i]){
				case ',':
					if(!value_box)
						return nullptr;

					continue;

				case ']':
					// End of the array.
					return root;

				default:
					// Anything else present here is an error.
					return nullptr;
			}
		}

		// We should have found a closing bracket. If we reach here this array
		// is invalid/incomplete.
		return nullptr;
	}

	// Return a map built from the JSON string provided.
	static Json::Object_p parseObject(std::string const& blob, int &i){
		root_p root = std::make_shared<root_t>();

		for(; i < blob.size(); ++ i){
			if(!skipWhitespace(blob, i))
				return nullptr;

			switch(blob[i]){
				case ',':
					continue;

				case '}':
					// We have reached the end of this structure.
					return std::make_shared<Json::Object>(root);

				case '"':
				{
					// Open quote starts a key name.
					std::string key;

					// Find the matching end quote, get the key name.
					{
						std::string const keysource(blob.substr(i + 1));
						size_t n = keysource.find('"');

						if(n != std::string::npos){
							key = keysource.substr(0, n);
							i += n + 2;

							// String ends sooner than expected!
							if(i >= blob.size())
								return nullptr;
						}
					}

					if(!skipWhitespace(blob, i))
						return nullptr;

					// If colon is missing or the string ends, stop immediately and fail parse.
					if((blob[i++] != ':') || (i >= blob.size()))
						return nullptr;

					Json::Value_p value_box = parseValue(blob, i);
					if(value_box){
						// Store the value
						(*root)[key] = value_box;
					}

					break;
				}

				default:
					return nullptr;
			}
		}

		// We should have returned from a closing curly brace, so reaching this
		// point is an error.
		return nullptr;
	}

public:
	// Parse any JSON value.
	static Json::Value_p parseValue(std::string const& blob){
		// Start from the beginning of the string.
		int resume_index = 0;

		return parseValue(blob, resume_index);
	}

	// Expect that the JSON string is a valid Object, return nullptr otherwise.
	static Json::Object_p parseObject(std::string const& blob){
		Json::Value_p v = parseValue(blob);

		if(!v || (v->type() != Json::Value::JSON_VALUE_OBJECT))
			return nullptr;

		return std::static_pointer_cast<Json::Object>(v);
	}
};

std::string Json::Value::str() const {
	std::stringstream ss;

	switch(type()){
		case Json::Value::JSON_VALUE_STRING:
			ss << "\"" << ((Json::String*) this)->get() << "\"";
			break;

		case Json::Value::JSON_VALUE_OBJECT:
			ss << ((Json::Object*) this)->str();
			break;

		case Json::Value::JSON_VALUE_BOOL:
			ss << (((Json::Bool*) this)->get() ? "true" : "false");
			break;

		case Json::Value::JSON_VALUE_NULL:
			ss << "null";
			break;

		case Json::Value::JSON_VALUE_INT:
			ss << ((Json::Int*) this)->get();
			break;

		case Json::Value::JSON_VALUE_ARRAY:
			{
				Json::Array* arr = ((Json::Array*) this);
				if(arr){
					bool first = true;

					ss << "[";

					for(int i = 0; i < arr->size(); ++ i){
						if(first){
							first = false;
						} else {
							ss << ",";
						}

						ss << arr->get(i)->str();
					}

					ss << "]";
				}
			}
			break;

		default:
			ss << "error";
	}

	return ss.str();
}

#endif
