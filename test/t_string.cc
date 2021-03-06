/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2012 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <typelib/compat.h>
#include <gtest/gtest.h>
#include <string>

class String : public ::testing::Test
{
};

TEST_F(String, testBasic)
{
    int rv;
    lcb_string str;
    rv = lcb_string_init(&str);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(NULL, str.base);
    ASSERT_EQ(0, str.nalloc);
    ASSERT_EQ(0, str.nused);

    rv = lcb_string_append(&str, "Hello", 5);
    ASSERT_EQ(0, rv);

    rv = lcb_string_append(&str, "blah", -1);
    ASSERT_EQ(-1, rv);

    rv = lcb_string_appendz(&str, "blah");
    ASSERT_EQ(0, rv);

    ASSERT_EQ(0, strcmp("Helloblah", str.base));

    lcb_string_erase_beginning(&str, 5);
    ASSERT_EQ(0, strcmp("blah", str.base));

    lcb_string_erase_end(&str, 4);
    ASSERT_EQ(0, strcmp("", str.base));

    lcb_string_clear(&str);
    ASSERT_TRUE(str.base != NULL);

    lcb_string_release(&str);
    ASSERT_EQ(NULL, str.base);
}

TEST_F(String, testAdvance)
{
    int rv;
    lcb_string str;
    rv = lcb_string_init(&str);
    ASSERT_EQ(0, rv);

    rv = lcb_string_reserve(&str, 30);
    ASSERT_EQ(0, rv);
    ASSERT_TRUE(str.nalloc >= 30);
    ASSERT_EQ(0, str.nused);

    memcpy(lcb_string_tail(&str), "Hello", 5);
    lcb_string_added(&str, 5);
    ASSERT_EQ(5, str.nused);
    ASSERT_EQ(0, strcmp(str.base, "Hello"));
    lcb_string_release(&str);
}

TEST_F(String, testFormat)
{
    int rv;
    tl_STRING str;
    ASSERT_EQ(0, tl_str_init(&str));
    std::string s(65536, '*');
    rv = tl_str_appendf(&str, "%s-%s-%s", s.c_str(), s.c_str(), s.c_str());
    ASSERT_EQ(0, rv);
    tl_str_cleanup(&str);
}

TEST_F(String, testReplace)
{
    int rv;
    tl_STRING str;
    tl_str_init(&str);
    tl_str_appendz(&str, "foofoofoo");
    rv = tl_str_substz(&str, "foo", "bar");
    ASSERT_EQ(0, rv);
    ASSERT_STREQ(str.base, "barbarbar");
    tl_str_cleanup(&str);

    // test with no match
    tl_str_init(&str);
    tl_str_appendz(&str, "nonmatching");
    rv = tl_str_substz(&str, "foo", "bar");
    ASSERT_EQ(0, rv);
    ASSERT_STREQ("nonmatching", str.base);
    tl_str_cleanup(&str);

    // test with empty replacement?
    tl_str_init(&str);
    tl_str_appendz(&str, "foofoofoo");
    rv = tl_str_substz(&str, "foo", "");
    ASSERT_EQ(0, rv);
    ASSERT_STREQ("", str.base);
    tl_str_cleanup(&str);

    // test with empty search pattern
    tl_str_init(&str);
    tl_str_appendz(&str, "foofoofoo");
    rv = tl_str_substz(&str, "", "bar");
    ASSERT_EQ(0, rv);
    ASSERT_STREQ("foofoofoo", str.base);
    tl_str_cleanup(&str);

    // test to remove test
    tl_str_init(&str);
    tl_str_appendz(&str, "~~REMOVEME~~");
    rv = tl_str_substz(&str, "REMOVEME", "");
    ASSERT_EQ(0, rv);
    ASSERT_STREQ(str.base, "~~~~");
    ASSERT_EQ(4, str.nused);
    tl_str_cleanup(&str);

    // test remove on empty string
    tl_str_init(&str);
    rv = tl_str_substz(&str, "nonexist", "");
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, str.nused);
    tl_str_cleanup(&str);
}

TEST_F(String, testSplit)
{
    char s[] = "foo,bar,baz";
    tl_STRLOC locs[3], *locp = locs;
    int nloc = 3;
    int rv;
    rv = tl_strsplit(s, ",", &locp, &nloc, TL_STRSPLIT_ZREPLACE);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(3, nloc);

    ASSERT_EQ(3, locs[0].length);
    ASSERT_EQ(3, locs[1].length);
    ASSERT_EQ(3, locs[2].length);

    ASSERT_STREQ("foo", locs[0].buf);
    ASSERT_STREQ("bar", locs[1].buf);
    ASSERT_STREQ("baz", locs[2].buf);

    // Test with standalone memory blocks
    rv = tl_strsplit(s, ",", &locp, &nloc, TL_STRSPLIT_DETACH);
    ASSERT_EQ(0, rv);
    ASSERT_STREQ("foo", locs[0].buf);
    ASSERT_STREQ("bar", locs[1].buf);
    ASSERT_STREQ("baz", locs[2].buf);
    for (int ii = 0; ii < nloc; ii++) {
        free(locs[ii].buf);
    }
}


TEST_F(String, testSplitEmpty)
{
    char s[] = "foo,bar,baz";
    int rv;
    tl_STRLOC *locs;
    int nloc = 0;
    rv = tl_strsplit(s, "", &locs, &nloc, 0);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, nloc);

    rv = tl_strsplit((char *)"", "blah", &locs, &nloc, 0);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, nloc);
}

TEST_F(String, testSplitMiss)
{
    char s[] = "foo,bar,baz";
    int rv, nloc = 0;
    tl_STRLOC *locs;
    rv = tl_strsplit(s, "|", &locs, &nloc, 0);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, nloc);
}

TEST_F(String, testSplitEmptyResults)
{
    char s[] = ",,,,";
    int rv, nloc = 0;
    tl_STRLOC *locs;
    rv = tl_strsplit(s, ",", &locs, &nloc, 0);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(5, nloc);
    for (int ii = 0; ii < nloc; ii++) {
        ASSERT_EQ(0, locs[ii].length);
    }
    free(locs);

    // try it again, but with detachable strings
    nloc = 0;
    rv = tl_strsplit(s, ",", &locs, &nloc, TL_STRSPLIT_DETACH);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(5, nloc);
    for (int ii = 0; ii < nloc; ii++) {
        ASSERT_EQ(0, locs[ii].length);
        ASSERT_EQ('\0', locs[ii].buf[0]);
        free(locs[ii].buf);
    }
    free(locs);

    // try it again, but with nul-termination
    nloc = 0;
    rv = tl_strsplit(s, ",", &locs, &nloc, TL_STRSPLIT_ZREPLACE);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(5, nloc);
    for (int ii = 0; ii < nloc; ii++) {
        ASSERT_EQ(0, locs[ii].length);
        ASSERT_EQ('\0', locs[ii].buf[0]);
    }
    free(locs);
}

TEST_F(String, testSplitUserLoc)
{
    char s[] = "foo,bar,baz";
    int rv, nloc = 2;
    tl_STRLOC loc[3], *locp = loc;
    rv = tl_strsplit(s, ",", &locp, &nloc, 0);
    ASSERT_NE(0, rv);
    ASSERT_EQ(-1, nloc);
}
