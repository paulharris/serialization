/*****************************************************************************/
/**
 * \file portable_oarchive.hpp
 * \brief Provides an archive to create portable binary files.
 * \author christian.pfligersdorffer@eos.info
 * \version 4.2
 *
 * This archive (pair) brings the advantanges of binary streams to the cross
 * platform boost::serialization user. While being almost as fast as the native
 * binary archive it allows its files to be exchanged between cpu architectures
 * using different byte order (endianness). Speaking of speed: in serializing
 * numbers the (portable) binary approach is approximately ten times faster than
 * the ascii implementation (that is inherently portable)!
 *
 * Based on the portable archive example by Robert Ramey this implementation
 * uses Beman Dawes endian library and fp_utilities from Johan Rade, both being
 * in boost since 1.36. Prior to that you need to add them both (header only)
 * to your boost directory before you're able to use the archives provided. 
 * Our archives have been tested successfully for boost versions 1.33 to 1.46!
 *
 * \note Correct behaviour has so far been confirmed using PowerPC-32, x86-32
 *       and x86-64 platforms featuring different byte order. So there is a good
 *       chance it will instantly work for your specific setup. If you encounter
 *       problems or have suggestions please contact the author.
 *
 * \note Version 4.2 maintains compatibility with the latest boost 1.46 and adds
 *       serialization of special floating point values inf and NaN as proposed
 *       by Francois Mauger.
 *
 * \note Version 4.1 makes the archives work together with boost 1.40 and 1.41.
 *       Thanks to Francois Mauger for his suggestions.
 *
 * \note Version 4 removes one level of the inheritance hierachy and directly
 *       builds upon binary primitive and basic binary archive, thereby fixing
 *       the last open issue regarding array serialization. Thanks to Robert
 *       Ramey for the hint.
 *
 * \note A few fixes introduced in version 3.1 let the archives pass all of the
 *       serialization tests. Thanks to Sergey Morozov for running the tests.
 *       Wouter Bijlsma pointed out where to find the fp_utilities and endian
 *       libraries headers inside the boost distribution. I would never have
 *       found them so thank him it works out of the box since boost 1.36.
 *
 * \note With Version 3.0 the archives have been made portable across different
 *       boost versions. For that purpose a header is added to the data that
 *       supplies the underlying serialization library version. Backwards
 *       compatibility is maintained by assuming library version boost 1.33 if
 *       the iarchive is created using the no_header flag. Whether a header is
 *       present or not can be guessed by peeking into the stream: the header's
 *       first byte is the magic number 127 coinciding with 'e' | 'o' | 's' :-)
 *
 * \note Version 2.1 removes several compiler warnings and enhances floating
 *       point diagnostics to inform the user if some preconditions are violated
 *		 on his platform. We do not strive for the universally portable solution
 *       in binary floating point serialization as desired by some boost users.
 *       Instead we support only the most widely used IEEE 754 format and try to
 *       detect when requirements are not met and hence our approach must fail.
 *       Contributions we made by Johan Rade and �kos Mar�y.
 *
 * \note Version 2.0 fixes a serious bug that effectively transformed most
 *       of negative integral values into positive values! For example the two
 *       numbers -12 and 234 were stored in the same 8-bit pattern and later
 *       always restored to 234. This was fixed in this version in a way that
 *       does not change the interpretation of existing archives that did work
 *       because there were no negative numbers. The other way round archives
 *       created by version 2.0 and containing negative numbers will raise an
 *       integer type size exception when reading it with version 1.0. Thanks
 *       to Markus Frohnmaier for testing the archives and finding the bug.
 *
 * \copyright The boost software license applies.
 */
/*****************************************************************************/

#pragma once

#include <ostream>

// basic headers
#include <boost/version.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/archive/basic_binary_oprimitive.hpp>
#include <boost/archive/basic_binary_oarchive.hpp>

#if BOOST_VERSION >= 103500
#include <boost/archive/shared_ptr_helper.hpp>
#endif

// funny polymorphics
#if BOOST_VERSION < 103500
#include <boost/archive/detail/polymorphic_oarchive_impl.hpp>
#define POLYMORPHIC(T) boost::archive::detail::polymorphic_oarchive_impl<T>

#elif BOOST_VERSION < 103600
#include <boost/archive/detail/polymorphic_oarchive_dispatch.hpp>
#define POLYMORPHIC(T) boost::archive::detail::polymorphic_oarchive_dispatch<T>

#else
#include <boost/archive/detail/polymorphic_oarchive_route.hpp>
#define POLYMORPHIC(T) boost::archive::detail::polymorphic_oarchive_route<T>
#endif

// endian and fpclassify
#if BOOST_VERSION < 103600
#include <boost/integer/endian.hpp>
#include <boost/math/fpclassify.hpp>
#elif BOOST_VERSION < 104900
#include <boost/spirit/home/support/detail/integer/endian.hpp>
#include <boost/spirit/home/support/detail/math/fpclassify.hpp>
#else
#include <boost/spirit/home/support/detail/endian.hpp>
#include <boost/spirit/home/support/detail/math/fpclassify.hpp>
#endif

// namespace alias
#if BOOST_VERSION < 103800
namespace fp = boost::math;
#else
namespace fp = boost::spirit::math;
#endif

// generic type traits for numeric types
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_floating_point.hpp>

#include "portable_archive_exception.hpp"

// hint from Johan Rade: on VMS there is still support for
// the VAX floating point format and this macro detects it
#if defined(__vms) && defined(__DECCXX) && !__IEEE_FLOAT
#error "VAX floating point format is not supported!"
#endif

namespace eos {

	// forward declaration
	class portable_oarchive;

#if BOOST_VERSION < 104900
   namespace endian_ns = boost::detail;
#else
   namespace endian_ns = boost::spirit::detail;
#endif


	typedef boost::archive::basic_binary_oprimitive<
		portable_oarchive
	#if BOOST_VERSION < 103400
		, std::ostream
	#else
		, std::ostream::char_type 
		, std::ostream::traits_type
	#endif
	> portable_oprimitive;

	/**
	 * \brief Portable binary output archive using little endian format.
	 *
	 * This archive addresses integer size, endianness and floating point types so
	 * that data can be transferred across different systems. The archive consists
	 * primarily of three different save implementations for integral types,
	 * floating point types and string types. Those functions are templates and use
	 * enable_if to be correctly selected for overloading.
	 *
	 * \note The class is based on the portable binary example by Robert Ramey and
	 *       uses Beman Dawes endian library plus fp_utilities by Johan Rade.
	 */
	class portable_oarchive : public portable_oprimitive

		// the example derives from common_oarchive but that lacks the
		// save_override functions so we chose to stay one level higher
		, public boost::archive::basic_binary_oarchive<portable_oarchive>

	#if BOOST_VERSION >= 103500
		// mix-in helper class for serializing shared_ptr
		, public boost::archive::detail::shared_ptr_helper
	#endif
	{
		// workaround for gcc: use a dummy struct
		// as additional argument type for overloading
		template<int> struct dummy { dummy(int) {}};

		// stores a signed char directly to stream
		void save_signed_char(const signed char& c) 
		{ 
			portable_oprimitive::save(c); 
		}

		// archive initialization
		void init(unsigned flags)
		{
			// it is vital to have version information if the archive is
			// to be parsed with a newer version of boost::serialization
			// therefor we create a header, no header means boost 1.33
			if (flags & boost::archive::no_header)
				BOOST_ASSERT(archive_version == 3);
			else
			{
				// write our minimalistic header (magic byte plus version)
				// the boost archives write a string instead - by calling
				// boost::archive::basic_binary_oarchive<derived_t>::init()
				save_signed_char(magic_byte);

				// write current version
//				save<unsigned>(archive_version);
				operator<<(archive_version);
			}
		}

	public:
		//! Save string types.
		template<class C, class T, class A>
		void save(const std::basic_string<C, T, A>& s)
		{
			// implementation only valid for narrow string
			BOOST_STATIC_ASSERT(sizeof(C) == sizeof(char));
			portable_oprimitive::save(s);
		}

		//! Saving bool directly, not by const reference
		//! because of tracking_type's operator (bool)
		void save(bool b)
		{
			save_signed_char(b);
			if (b) save_signed_char('T');
		}

		/**
		 * \brief Save integer types.
		 *
		 * First we save the size information ie. the number of bytes that hold the
		 * actual data. We subsequently transform the data using store_little_endian
		 * and store non-zero bytes to the stream.
		 */
		template <typename T>
		typename boost::enable_if<boost::is_integral<T> >::type
		save(const T & t, dummy<2> = 0)
		{
			if (T temp = t)
			{
				// examine the number of bytes
				// needed to represent the number
				signed char size = 0;
				do { temp >>= CHAR_BIT; ++size; } 
				while (temp != 0 && temp != (T) -1);

				// encode the sign bit into the size
				save_signed_char(t > 0 ? size : -size);
				BOOST_ASSERT(t > 0 || boost::is_signed<T>::value);

				// we choose to use little endian because this way we just
				// save the first size bytes to the stream and skip the rest
				endian_ns::store_little_endian<T, sizeof(T)>(&temp, t);
				save_binary(&temp, size);
			}
			// zero optimization
			else save_signed_char(0);
		}

		/**
		 * \brief Save floating point types.
		 * 
		 * We simply rely on fp_traits to extract the bit pattern into an (unsigned)
		 * integral type and store that into the stream. Francois Mauger provided
		 * standardized behaviour for special values like inf and NaN, that need to
		 * be serialized in his application.
		 *
		 * \note by Johan Rade (author of the floating point utilities library):
		 * Be warned that the math::detail::fp_traits<T>::type::get_bits() function 
		 * is *not* guaranteed to give you all bits of the floating point number. It
		 * will give you all bits if and only if there is an integer type that has
		 * the same size as the floating point you are copying from. It will not
		 * give you all bits for double if there is no uint64_t. It will not give
		 * you all bits for long double if sizeof(long double) > 8 or there is no
		 * uint64_t. 
		 * 
		 * The member fp_traits<T>::type::coverage will tell you whether all bits
		 * are copied. This is a typedef for either math::detail::all_bits or
		 * math::detail::not_all_bits. 
		 * 
		 * If the function does not copy all bits, then it will copy the most
		 * significant bits. So if you serialize and deserialize the way you
		 * describe, and fp_traits<T>::type::coverage is math::detail::not_all_bits,
		 * then your floating point numbers will be truncated. This will introduce
		 * small rounding off errors. 
		 */
		template <typename T>
		typename boost::enable_if<boost::is_floating_point<T> >::type
		save(const T & t, dummy<3> = 0)
		{
			typedef typename fp::detail::fp_traits<T>::type traits;

			// if the no_infnan flag is set we must throw here
			if (get_flags() & no_infnan && !fp::isfinite(t))
				throw portable_archive_exception(t);

			// if you end here there are three possibilities:
			// 1. you're serializing a long double which is not portable
			// 2. you're serializing a double but have no 64 bit integer
			// 3. your machine is using an unknown floating point format
			// after reading the note above you still might decide to 
			// deactivate this static assert and try if it works out.
			typename traits::bits bits;
			BOOST_STATIC_ASSERT(sizeof(bits) == sizeof(T));
			BOOST_STATIC_ASSERT(std::numeric_limits<T>::is_iec559);

			// examine value closely
			switch (fp::fpclassify(t))
			{
			case FP_ZERO: bits = 0; break;
			case FP_NAN: bits = traits::exponent | traits::mantissa; break;
			case FP_INFINITE: bits = traits::exponent | (t<0) * traits::sign; break;
			case FP_SUBNORMAL: assert(std::numeric_limits<T>::has_denorm);
			case FP_NORMAL: traits::get_bits(t, bits); break;
			default: throw portable_archive_exception(t);
			}

			save(bits);
		}

		/**
		 * \brief Constructor on a stream using ios::binary mode!
		 *
		 * We cannot call basic_binary_oprimitive::init which stores type
		 * sizes to the archive in order to detect tranfers to non-compatible
		 * platforms.
		 *
		 * We could have called basic_binary_oarchive::init which would create
		 * the boost::serialization standard archive header containing also the
		 * library version. Due to efficiency we stick with our own.
		 */
		portable_oarchive(std::ostream& os, unsigned flags = 0)
		#if BOOST_VERSION < 103400
			: portable_oprimitive(os, flags & boost::archive::no_codecvt)
		#else
			: portable_oprimitive(*os.rdbuf(), flags & boost::archive::no_codecvt)
		#endif
			, boost::archive::basic_binary_oarchive<portable_oarchive>(flags)
		{
			init(flags);
		}

	#if BOOST_VERSION >= 103400
		portable_oarchive(std::streambuf& sb, unsigned flags = 0)
			: portable_oprimitive(sb, flags & boost::archive::no_codecvt)
			, boost::archive::basic_binary_oarchive<portable_oarchive>(flags)
		{
			init(flags);
		}
	#endif

	#if BOOST_VERSION >= 104400
		// in boost 1.44 version_type was splitted into library_version_type and
		// item_version_type, plus a whole bunch of additional strong typedefs
		void save(const boost::archive::library_version_type& version)
		{
			save((boost::uint_least16_t)(version));
		}

		void save(const boost::archive::class_id_type& class_id)
		{
			save((boost::uint_least16_t)(class_id)); // has operator (int) ??!
		}

		void save(const boost::serialization::item_version_type& class_id)
		{
			save((boost::uint_least32_t)(class_id));
		}

		void save(const boost::serialization::collection_size_type& class_id)
		{
			save((boost::uint_least32_t)(class_id));
		}

		void save(const boost::archive::object_id_type& object_id)
		{
			save((boost::uint_least32_t)(object_id));
		}

		void save(const boost::archive::version_type& version)
		{
			save((boost::uint_least32_t)(version));
		}
	#endif
	};

	// polymorphic portable binary oarchive typedef
	typedef POLYMORPHIC(portable_oarchive) polymorphic_portable_oarchive;
	#undef POLYMORPHIC

} // namespace eos

// required by export
#if BOOST_VERSION < 103500
#define BOOST_ARCHIVE_CUSTOM_OARCHIVE_TYPES eos::portable_oarchive
#else
BOOST_SERIALIZATION_REGISTER_ARCHIVE(eos::portable_oarchive)
BOOST_SERIALIZATION_REGISTER_ARCHIVE(eos::polymorphic_portable_oarchive)
#endif

// if you include this header multiple times and your compiler is picky
// about multiple template instantiations (eg. gcc is) then you need to
// define NO_EXPLICIT_TEMPLATE_INSTANTIATION before every include but one
// or you move the instantiation section into an implementation file
#ifndef NO_EXPLICIT_TEMPLATE_INSTANTIATION

#include <boost/archive/impl/basic_binary_oarchive.ipp>
#include <boost/archive/impl/basic_binary_oprimitive.ipp>

#if BOOST_VERSION < 104000
#include <boost/archive/impl/archive_pointer_oserializer.ipp>
#elif !defined BOOST_ARCHIVE_SERIALIZER_INCLUDED
#include <boost/archive/impl/archive_serializer_map.ipp>
#define BOOST_ARCHIVE_SERIALIZER_INCLUDED
#endif

namespace boost { namespace archive {

	// explicitly instantiate for this type of binary stream
	template class basic_binary_oarchive<eos::portable_oarchive>;

	template class basic_binary_oprimitive<
		eos::portable_oarchive
	#if BOOST_VERSION < 103400
		, std::ostream
	#else
		, std::ostream::char_type
		, std::ostream::traits_type
	#endif
	>;

#if BOOST_VERSION < 104000
	template class detail::archive_pointer_oserializer<eos::portable_oarchive>;
#else
	template class detail::archive_serializer_map<eos::portable_oarchive>;
	//template class detail::archive_serializer_map<eos::polymorphic_portable_oarchive>;
#endif

} } // namespace boost::archive

#endif
