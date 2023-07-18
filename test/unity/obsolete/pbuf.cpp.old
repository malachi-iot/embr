typedef embr::lwip::legacy::PbufNetbuf netbuf_type;
typedef netbuf_type::size_type size_type;


TEST_CASE("lwip pbuf embr-netbuf: out streambuf", "[lwip-pbuf]")
{
    // NOTE: Not tested with move constructor, only tested with reference version
    netbuf_type _netbuf(netbuf_size);

    TEST_ASSERT(_netbuf.size() == netbuf_size);

    //out_netbuf_streambuf<char, embr::lwip::PbufNetbuf&> sb(_netbuf);
    out_netbuf_streambuf<char, netbuf_type> sb(std::move(_netbuf));
    //out_netbuf_streambuf<char, embr::lwip::PbufNetbuf> sb(netbuf_size);

    sb.xsputn(s1, s1_size);

    auto sz = sb.pptr() - sb.pbase();

    TEST_ASSERT(sz == s1_size);

    // FIX: None of these work
    //estd::layer2::string<s1_size, false> _s((char*)_netbuf.data());
    estd::layer2::string<s1_size, false> _s(sb.pbase());
    //estd::layer2::string<s1_size> _s(s1);
    //estd::layer2::basic_string<const char, s1_size> _s((char*)_netbuf.data());

    ESP_LOGI(TAG, "sz = %d, _s.size() = %d", sz, _s.size());

    // FIX: _s.size() comes out to 0 somehow
    //TEST_ASSERT(_s.size() == s1_size);

    estd::layer3::const_string s(sb.pbase(), s1_size);
    //estd::layer2::const_string s(s1);

    TEST_ASSERT(s == s1);
}


TEST_CASE("lwip pbuf embr-netbuf: ostream", "[lwip-pbuf]")
{
    // NOTE: Compiles, not runtime tested at all
    typedef out_netbuf_streambuf<char, netbuf_type> streambuf_type;

    estd::internal::basic_ostream<streambuf_type> 
        out(netbuf_size);

    out << s1;

    auto sb = out.rdbuf();

    estd::layer3::const_string s(sb->pbase(), s1_size);

    ESP_LOGI(TAG, "sz = %d", sb->pptr() - sb->pbase());

    TEST_ASSERT(s == s1);
}




#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
TEST_CASE("lwip pbuf embr-netbuf: out streambuf chain", "[lwip-pbuf]")
{
    constexpr int netbuf_size = 64;

    out_pbuf_streambuf sb(netbuf_size);
    
    const netbuf_type& netbuf = sb.cnetbuf();

    ESP_LOGI(TAG, "#1 total_size = %d", netbuf.total_size());
    TEST_ASSERT(netbuf.total_size() == netbuf_size);

    for(int i = 0; i < 1 + (netbuf_size * 3); i += s1_size)
    {
        sb.sputn(s1, s1_size);
    }

    ESP_LOGI(TAG, "#2 total_size = %d", netbuf.total_size());
    TEST_ASSERT(netbuf.total_size() == ((netbuf_size * 3) + netbuf_type::threshold_size));
}
#endif


#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
// discere input vs output streambufs
TEST_CASE("lwip pbuf embr-netbuf: out+in streambuf chain 1", "[lwip-pbuf]")
{
    constexpr int netbuf_size = 64;

    out_pbuf_streambuf sb_out(netbuf_size);
    
    const netbuf_type& out_netbuf = sb_out.cnetbuf();

    for(int i = 0; i < 1 + (netbuf_size * 3); i += s1_size)
    {
        sb_out.sputn(s1, s1_size);
    }

    in_pbuf_streambuf sb(out_netbuf, true);

    const netbuf_type& in_netbuf = sb.cnetbuf();

    TEST_ASSERT_EQUAL_INT(0, sb.pos());
    TEST_ASSERT_EQUAL_INT(netbuf_size, in_netbuf.size());

    char buf[netbuf_size];

    // NOTE: Still figuring out if this is total size or chunk size
    // at the moment, it's total size
    TEST_ASSERT_EQUAL_INT(224, sb.in_avail());

    int read_back = sb.sbumpc();

    TEST_ASSERT(sb.pos() == 1);
    
    TEST_ASSERT(read_back == s1[0]);

    read_back = sb.sgetn(buf, netbuf_size / 2);

    TEST_ASSERT_EQUAL_INT(1 + netbuf_size / 2, sb.pos());

    TEST_ASSERT(read_back == netbuf_size / 2);
}
#endif


#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
TEST_CASE("lwip pbuf embr-netbuf: netbuf shrink", "[lwip-pbuf]")
{
    // UNFINISHED
    size_type constexpr threshold_size = netbuf_type::threshold_size;
    int rotation_size = 50;
    int rotations = 6; // presuming these are gonna be bigger than threshold size, for now

    opbufstream out(threshold_size);

    // assumes ASCII
    const char ch = '0';

    // populate pbuf with ASCII stuff
    for(int count = 0; count < rotations; count++)
        for(int r = 0; r < rotation_size; r++)
            out.put(ch + r);

    netbuf_type& netbuf = out.rdbuf()->netbuf();

    size_type total_size = netbuf.total_size();
    size_type actual_size = out.tellp();

    TEST_ASSERT_EQUAL((rotation_size * rotations), actual_size);
    TEST_ASSERT_LESS_OR_EQUAL(total_size, actual_size);

    netbuf.shrink(actual_size);
    netbuf.reset();

    ipbufstream in(netbuf.pbuf());

    // read same pbuf out via istream looking for same ASCII
    for(int count = 0; count < rotations; count++)
        for(int r = 0; r < rotation_size; r++)
        {
            char ch_expected = ch + r;
            char ch_actual = in.get();

            TEST_ASSERT_EQUAL(ch_expected, ch_actual);
        }

    TEST_ASSERT_EQUAL(traits_type::eof(), in.get());
}
#endif
