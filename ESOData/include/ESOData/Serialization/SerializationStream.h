#ifndef ESODATA_SERIALIZATION_SERIALIZATION_STREAM_H
#define ESODATA_SERIALIZATION_SERIALIZATION_STREAM_H

#include <stdint.h>

#include <type_traits>
#include <vector>
#include <string>

namespace esodata {
	class SerializationStream {
	protected:
		SerializationStream();
		~SerializationStream();

	public:
		SerializationStream(const SerializationStream &other) = delete;
		SerializationStream &operator =(const SerializationStream &other) = delete;

		inline bool swapEndian() const {
			return m_swapEndian;
		}

		inline void setSwapEndian(bool swapEndian) {
			m_swapEndian = swapEndian;
		}

		void writeArithmetic(const unsigned char *data, size_t dataSize);
		void readArithmetic(unsigned char *data, size_t dataSize);

		void writeFloat(const unsigned char* data, size_t dataSize);
		void readFloat(unsigned char* data, size_t dataSize);

		void writeData(const unsigned char *data, size_t dataSize);
		void readData(unsigned char *data, size_t dataSize);

		virtual unsigned char *getRegionForWrite(size_t size) = 0;
		virtual const unsigned char *getRegionForRead(size_t size) = 0;

		virtual size_t getCurrentPosition() const = 0;
		virtual void setCurrentPosition(size_t position) = 0;

	private:
		bool m_swapEndian;
	};

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, SerializationStream &>::type operator <<(SerializationStream &stream, T value) {
		stream.writeArithmetic(reinterpret_cast<const unsigned char *>(&value), sizeof(value));
		return stream;
	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, SerializationStream &>::type operator >>(SerializationStream &stream, T &value) {
		stream.readArithmetic(reinterpret_cast<unsigned char *>(&value), sizeof(value));
		return stream;
	}

	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value, SerializationStream&>::type operator <<(SerializationStream& stream, T value) {
		stream.writeFloat(reinterpret_cast<const unsigned char*>(&value), sizeof(value));
		return stream;
	}

	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value, SerializationStream&>::type operator >>(SerializationStream& stream, T& value) {
		stream.readFloat(reinterpret_cast<unsigned char*>(&value), sizeof(value));
		return stream;
	}

	template<typename T>
	typename std::enable_if<std::is_enum<T>::value, SerializationStream &>::type operator <<(SerializationStream &stream, T value) {
		return stream << static_cast<std::underlying_type<T>::type>(value);
	}
	
	template<typename T>
	typename std::enable_if<std::is_enum<T>::value, SerializationStream &>::type operator >>(SerializationStream &stream, T &value) {
		typename std::underlying_type<T>::type tmpval;
		stream >> tmpval;
		value = static_cast<T>(tmpval);
		return stream;
	}

	template<typename T>
	SerializationStream &operator <<(SerializationStream &stream, const std::vector<T> &value) {
		for (const auto &entry : value) {
			stream << entry;
		}

		return stream;
	}

	template<>
	SerializationStream& operator <<<bool>(SerializationStream& stream, const std::vector<bool>& value);

	template<typename T>
	SerializationStream &operator >>(SerializationStream &stream, std::vector<T> &value) {
		for (auto &entry : value) {
			stream >> entry;
		}

		return stream;
	}

	template<>
	SerializationStream& operator >><bool>(SerializationStream& stream, std::vector<bool>& value);

	template<typename T, size_t N>
	SerializationStream &operator <<(SerializationStream &stream, const std::array<T, N> &value) {
		for (auto &entry : value) {
			stream << entry;
		}

		return stream;
	}

	template<typename T, size_t N>
	SerializationStream &operator >>(SerializationStream &stream, std::array<T, N> &value) {
		for (auto &entry : value) {
			stream >> entry;
		}

		return stream;
	}

	template<size_t N>
	SerializationStream &operator <<(SerializationStream &stream, const std::array<unsigned char, N> &value) {
		stream.writeData(value.data(), value.size());

		return stream;
	}

	template<size_t N>
	SerializationStream &operator >>(SerializationStream &stream, std::array<unsigned char, N> &value) {
		stream.readData(value.data(), value.size());

		return stream;
	}

	template<>
	SerializationStream &operator <<(SerializationStream &stream, const std::vector<unsigned char> &value);

	template<>
	SerializationStream &operator >>(SerializationStream &stream, std::vector<unsigned char> &value);

	template<>
	SerializationStream &operator <<(SerializationStream &stream, const std::vector<char> &value);

	template<>
	SerializationStream &operator >>(SerializationStream &stream, std::vector<char> &value);

	SerializationStream &operator <<(SerializationStream &stream, const std::string &value);
	SerializationStream &operator >>(SerializationStream &stream, std::string &value);

	SerializationStream& operator <<(SerializationStream& stream, bool value);
	SerializationStream& operator >>(SerializationStream& stream, bool &value);

}

#endif
