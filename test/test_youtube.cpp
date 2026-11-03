#include "yt_youtube.h"
#include "yt_media.h"
#include <iostream>

static int failures=0;
static void check(bool ok,const char* message) {if(!ok){std::cerr<<"FAIL: "<<message<<'\n';++failures;}}
static std::string bootstrap() {
    return R"(<script>ytcfg.set({"INNERTUBE_CONTEXT":{"client":{"clientName":"WEB","clientVersion":"test","visitorData":"visitor"}},"INNERTUBE_API_KEY":"key"});var ytInitialData = {"contents":[{"itemSectionRenderer":{"sectionIdentifier":"comment-item-section","contents":[{"continuationItemRenderer":{"continuationEndpoint":{"continuationCommand":{"token":"initial"}}}}]}}]};var ytInitialPlayerResponse = {"videoDetails":{"videoId":"aqz-KE-bpKQ","title":"A } brace"}};</script>)";
}
static std::string page(const std::string& items,const std::string& mutations="[]") {
    return "{\"frameworkUpdates\":{\"entityBatchUpdate\":{\"mutations\":"+mutations+"}},\"onResponseReceivedEndpoints\":[{\"appendContinuationItemsAction\":{\"continuationItems\":"+items+"}}]}";
}
int main() {
    check(ytwe::text(yj::parse(R"({"runs":[{"text":"one"},{"text":" two"}]})"))=="one two","rich text concatenation");
    check(ytwe::count_text("1,234 likes")==1234,"a11y likes");
    check(ytwe::count_text("1.2K")==1200,"abbreviated likes");
    check(ytwe::count_text("No likes")==-1,"missing count remains unknown");
    check(ytwe::base64("abc")=="YWJj","base64");
    check(ytwe::url_decode("a%2Bb+c")=="a+b c","cipher URL percent decoding");
    check(ytwe::set_query("https://host/a?n=old&x=1","n","new+value")=="https://host/a?n=new%2Bvalue&x=1","n query replacement preserves other arguments");
    auto resolved=ytwe::resolve_player_json(R"({"playabilityStatus":{"status":"OK"},"streamingData":{"adaptiveFormats":[{"itag":140,"mimeType":"audio/mp4","signatureCipher":"url=https%3A%2F%2Fhost%2Fmedia%3Fn%3Dthrottled%26x%3D1&s=encrypted&sp=sig"},{"itag":999}]}})",
        [&](const std::vector<std::string>& n,const std::vector<std::string>& sig){
            check(n==std::vector<std::string>{"throttled"}&&sig==std::vector<std::string>{"encrypted"},"challenge batching");
            return std::unordered_map<std::string,std::string>{{"n:throttled","solved-n"},{"sig:encrypted","solved-sig"}};
        });
    auto resolved_formats=yj::path(yj::parse(resolved),"streamingData","adaptiveFormats");
    check(yj::count(resolved_formats)==1,"URL-less format excluded");
    yj::each_elem(resolved_formats,[&](yj::Val f){
        check(ytwe::str(yj::get(f,"url"))=="https://host/media?n=solved-n&x=1&sig=solved-sig","native signature/n URL rewriting");
        check(!yj::get(f,"signatureCipher"),"resolved formats no longer carry cipher field");return true;
    });
    check(ytwe::comment_continuation("aqz-KE-bpKQ")=="Eg0SC2Fxei1LRS1icEtRGAYyJyIRIgthcXotS0UtYnBLUTAAeAIwAEIQY29tbWVudHMtc2VjdGlvbg==","upstream protobuf continuation");
    check(ytwe::embedded_json("var x = {\"title\":\"brace } inside\"};","x =")=="{\"title\":\"brace } inside\"}","embedded JSON respects strings");
    std::vector<std::string> requests;
    ytwe::Youtube yt([&](const std::string& url,const std::string& body,const std::vector<std::string>&){
        if(body.empty())return bootstrap();
        check(url.find("/next?")!=std::string::npos,"comments endpoint");
        check(yj::path(yj::parse(body),"context","client","clientVersion").raw()=="test","WEB context reused");
        auto token=ytwe::str(yj::get(yj::parse(body),"continuation"));requests.push_back(token);
        if(token=="initial")return page(R"([{"commentsHeaderRenderer":{"sortMenu":{"sortFilterSubMenuRenderer":{"subMenuItems":[{"serviceEndpoint":{"continuationCommand":{"token":"top"}}},{"serviceEndpoint":{"continuationCommand":{"token":"new"}}}]}}}}])");
        if(token=="new")return page(R"([{"commentThreadRenderer":{"commentViewModel":{"commentViewModel":{"commentKey":"parent-entity","toolbarStateKey":"toolbar","pinnedText":"Pinned"}},"replies":{"commentRepliesRenderer":{"contents":[{"continuationItemRenderer":{"continuationEndpoint":{"continuationCommand":{"token":"replies"}}}}]}}}},{"continuationItemRenderer":{"continuationEndpoint":{"continuationCommand":{"token":"page2"}}}}])",
            R"([{"entityKey":"parent-entity","payload":{"commentEntityPayload":{"properties":{"commentId":"parent","content":{"content":"Hello 😀"},"publishedTime":"1 day ago"},"author":{"channelId":"UC123","displayName":"Author","isCreator":true},"toolbar":{"likeCountA11y":"1,234 likes"}}}},{"entityKey":"toolbar","payload":{"engagementToolbarStateEntityPayload":{"heartState":"TOOLBAR_HEART_STATE_HEARTED"}}}])");
        if(token=="replies")return page(R"([{"commentRenderer":{"commentId":"reply","contentText":{"runs":[{"text":"Reply"}]},"authorText":{"simpleText":"Reader"},"voteCount":{"simpleText":"2"}}}])");
        if(token=="page2")return page(R"([{"commentRenderer":{"commentId":"parent","contentText":{"simpleText":"duplicate"}}},{"commentRenderer":{"commentId":"other","contentText":{"simpleText":"Other"}}}])");
        throw std::runtime_error("unexpected fixture token: "+token);
    });
    yt.bootstrap("aqz-KE-bpKQ");
    check(ytwe::str(yj::path(yj::parse(yt.player_json()),"videoDetails","title"))=="A } brace","bootstrap player JSON");
    std::vector<ytwe::Comment> comments;
    auto result=yt.comments("aqz-KE-bpKQ",{},[&](const ytwe::Comment& c){comments.push_back(c);});
    check(result.count==3&&result.pages==4&&result.exhausted,"pagination, replies and deduplication");
    check(requests==std::vector<std::string>({"initial","new","replies","page2"}),"newest sort and traversal order");
    check(comments.size()==3&&comments[0].hearted&&comments[0].pinned&&comments[0].uploader,"entity and toolbar metadata");
    check(comments.size()==3&&comments[0].likes==1234&&comments[1].parent=="parent","reply parent and count");
    ytwe::CommentOptions limit;limit.limit=1;comments.clear();
    result=yt.comments("aqz-KE-bpKQ",limit,[&](const ytwe::Comment& c){comments.push_back(c);});
    check(result.count==1&&!result.exhausted&&result.status=="limit","global comment cap");
    ytwe::CommentOptions page_limit;page_limit.max_pages=1;
    result=yt.comments("aqz-KE-bpKQ",page_limit,[](const ytwe::Comment&){});
    check(result.pages==1&&!result.exhausted&&result.status=="page_limit","page budget includes header");
    bool loop_detected=false;
    ytwe::Youtube loop([&](const std::string&,const std::string& body,const std::vector<std::string>&){
        if(body.empty())return bootstrap();
        return page(R"([{"continuationItemRenderer":{"continuationEndpoint":{"continuationCommand":{"token":"initial"}}}}])");
    });
    try{loop.comments("aqz-KE-bpKQ",{},[](const ytwe::Comment&){});}catch(const std::exception&){loop_detected=true;}
    check(loop_detected,"repeated token terminates");
    size_t top_round=0;
    ytwe::Youtube top([&](const std::string&,const std::string& body,const std::vector<std::string>&){
        if(body.empty())return bootstrap();
        if(++top_round==1)return page(R"([{"commentsHeaderRenderer":{"sortMenu":{"sortFilterSubMenuRenderer":{"subMenuItems":[{"serviceEndpoint":{"continuationCommand":{"token":"initial"}}}]}}}}])");
        return page(R"([{"commentRenderer":{"commentId":"top-comment","contentText":{"simpleText":"Top comment"}}}])");
    });
    ytwe::CommentOptions top_options;top_options.newest=false;
    auto top_result=top.comments("aqz-KE-bpKQ",top_options,[](const ytwe::Comment&){});
    check(top_result.count==1&&top_result.pages==2&&top_result.exhausted,"top-sort header may reuse initial token once");
    bool no_context=false;
    ytwe::Youtube bad([](const std::string&,const std::string&,const std::vector<std::string>&){return "<html>Consent required</html>";});
    try{bad.bootstrap("aqz-KE-bpKQ");}catch(const std::exception&){no_context=true;}
    check(no_context,"consent HTML is not silently treated as success");
    size_t playlist_requests=0;
    ytwe::Youtube playlist([&](const std::string&,const std::string& body,const std::vector<std::string>&){
        if(body.empty())return bootstrap();
        ++playlist_requests;
        if(playlist_requests==1)return std::string(R"({"contents":{"playlistVideoListRenderer":{"contents":[{"playlistVideoRenderer":{"videoId":"aqz-KE-bpKQ","title":{"simpleText":"One"},"lengthSeconds":"10"}},{"continuationItemRenderer":{"continuationEndpoint":{"continuationCommand":{"token":"playlist2"}}}}]}}})");
        return page(R"([{"playlistVideoRenderer":{"videoId":"aqz-KE-bpKQ","title":{"simpleText":"Repeated on purpose"},"lengthSeconds":"10"}}])");
    });
    playlist.bootstrap("aqz-KE-bpKQ");std::vector<ytwe::Youtube::PlaylistEntry> entries;
    auto pl=playlist.playlist("PLtest",100,10,[&](const auto& e){entries.push_back(e);});
    check(pl.count==2&&pl.pages==2&&pl.exhausted&&entries[1].title=="Repeated on purpose","playlist continuations preserve intentional repeats");
    std::cout<<(failures?"FAILED":"PASS")<<": native YouTube extractor ("<<failures<<" failures)\n";
    return failures?1:0;
}
