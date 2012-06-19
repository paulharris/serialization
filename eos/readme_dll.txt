You can include portable_iarchive.hpp and portable_oarchive.hpp directly into
your source, IF you have linked to boost statically.

Else, you need to put these files into the boost source,
compile them with boost,
and you must include portable_iarchive_dll.hpp / portable_oarchive_dll.hpp instead.

The procedure is half-done with the copy_into_boost.sh script (designed for cygwin),
which copies the headers into the header area,
and the .cpp into the serialization area.

Lastly, you must manually edit the file:
BOOST/libs/serialization/build/Jamfile.v2
and add "portable_archive_dll" to the SOURCES variable list.
