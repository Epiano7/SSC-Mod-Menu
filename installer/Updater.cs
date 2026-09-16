using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Security.Principal;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

namespace SSCMods.Setup {
    public sealed class ReleaseAsset {
        public string name, browser_download_url, digest, state;
        public long size;
    }
    public sealed class RemoteRelease {
        public string tag_name;
        public bool draft, prerelease;
        public ReleaseAsset[] assets;
    }
    public static class Updater {
        public const string CurrentVersion="0.1.1";
        const string Api="https://api.github.com/repos/Epiano7/SSC-Mod-Menu/releases/latest";
        const string Prefix="https://github.com/Epiano7/SSC-Mod-Menu/releases/download/";
        public static Version ParseVersion(string text) {
            if(!Regex.IsMatch(text??"",@"^v?\d{1,5}\.\d{1,5}\.\d{1,5}$"))throw new IOException("Invalid release version.");
            return Version.Parse(text.TrimStart('v'));
        }
        public static ReleaseAsset Select(string json,string current,out string version) {
            var release=new JavaScriptSerializer{MaxJsonLength=1048576}.Deserialize<RemoteRelease>(json);
            version=null;
            if(release==null||release.draft||release.prerelease)return null;
            var next=ParseVersion(release.tag_name);
            if(next<=ParseVersion(current))return null;
            var assets=(release.assets??new ReleaseAsset[0]).Where(a=>a!=null&&a.name=="SSC-Mod-Menu-Setup.exe").ToArray();
            if(assets.Length!=1)throw new IOException("Release installer is missing or ambiguous.");
            var asset=assets[0];string expected=Prefix+release.tag_name+"/SSC-Mod-Menu-Setup.exe";
            if(asset.browser_download_url!=expected||asset.state!="uploaded"||asset.size<1024||asset.size>134217728||!Regex.IsMatch(asset.digest??"",@"^sha256:[a-fA-F0-9]{64}$"))
                throw new IOException("Release installer metadata is invalid.");
            version=next.ToString();return asset;
        }
        static byte[] Fetch(string url,long limit) {
            ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;
            var request=(HttpWebRequest)WebRequest.Create(url);request.UserAgent="SSC-Mod-Menu/"+CurrentVersion;
            request.Accept="application/vnd.github+json";request.Timeout=20000;request.ReadWriteTimeout=20000;
            using(var response=(HttpWebResponse)request.GetResponse()) {
                if(response.ResponseUri.Scheme!="https"||response.ContentLength>limit)throw new IOException("Invalid update response.");
                using(var stream=response.GetResponseStream())using(var bytes=new MemoryStream()) {
                    byte[] buffer=new byte[65536];int count;var time=Stopwatch.StartNew();
                    while((count=stream.Read(buffer,0,buffer.Length))>0){if(time.ElapsedMilliseconds>180000||bytes.Length+count>limit)throw new IOException("Update download exceeded its limit.");bytes.Write(buffer,0,count);}
                    return bytes.ToArray();
                }
            }
        }
        static ReleaseAsset Latest(out string version) {return Select(Encoding.UTF8.GetString(Fetch(Api,1048576)),CurrentVersion,out version);}
        public static void Verify(byte[] bytes,ReleaseAsset asset) {
            if(bytes.LongLength!=asset.size||!String.Equals(Engine.Hash(bytes),asset.digest.Substring(7),StringComparison.OrdinalIgnoreCase))throw new IOException("Installer download failed verification.");
        }
        static string Job(string path) {
            string result=Path.GetFullPath(path);Engine.RejectLinks(result);
            if(!Directory.Exists(result))throw new IOException("Update job folder is missing.");
            return result;
        }
        public static void Report(string job,string message) {
            job=Job(job);string path=Path.Combine(job,"status.txt"),temp=Path.Combine(job,Guid.NewGuid().ToString("N")+".tmp");
            File.WriteAllText(temp,message.Replace("\r","").Replace("\n"," "),new UTF8Encoding(false));
            if(File.Exists(path))File.Replace(temp,path,null);else File.Move(temp,path);
        }
        public static Process Game(string root,int pid,long started) {
            var game=Process.GetProcessById(pid);
            try{
                if(game.HasExited||game.StartTime.ToUniversalTime().Ticks!=started||!String.Equals(game.MainModule.FileName,Path.Combine(Engine.Root(root),"SkillshotCity.exe"),StringComparison.OrdinalIgnoreCase))throw new IOException("The game session changed. Retry from its menu.");
                return game;
            }catch{game.Dispose();throw;}
        }
        public static void Check(string job) {
            try{string version;var asset=Latest(out version);Report(job,asset==null?"current":"available "+version);}
            catch(WebException e){var response=e.Response as HttpWebResponse;Report(job,response!=null&&response.StatusCode==HttpStatusCode.NotFound?"unavailable No public release is available yet.":"error Could not check GitHub. Try again later.");}
            catch(Exception){Report(job,"error Could not verify the release information.");}
        }
        public static bool IsAdministrator() {using(var identity=WindowsIdentity.GetCurrent())return new WindowsPrincipal(identity).IsInRole(WindowsBuiltInRole.Administrator);}
        public static bool NeedsElevation(string root) {
            root=Engine.Root(root);if(IsAdministrator())return false;
            string probe=Path.Combine(root,".ssc-write-test-"+Guid.NewGuid().ToString("N"));
            try{using(var file=new FileStream(probe,FileMode.CreateNew,FileAccess.ReadWrite,FileShare.None,1,FileOptions.DeleteOnClose)){}return false;}
            catch(UnauthorizedAccessException){return true;}
        }
        public static string Quote(string value){if(value.IndexOfAny(new[]{'"','\r','\n'})>=0)throw new IOException("Invalid argument.");return "\""+value.TrimEnd('\\')+"\"";}
        // The unelevated helper survives game exit, then launches Steam in the user's session.
        public static void DownloadAndApply(string root,int pid,long started,string job,string acceptedVersion) {
            job=Job(job);
            try{
                using(Game(root,pid,started)){}
                string version;var asset=Latest(out version);
                if(asset==null||version!=acceptedVersion)throw new IOException("The release changed. Check for updates again.");
                Report(job,"downloading Downloading update...");
                byte[] bytes=Fetch(asset.browser_download_url,asset.size);Verify(bytes,asset);
                string setup=Path.Combine(job,"SSC-Mod-Menu-Setup.exe");
                using(var file=new FileStream(setup,FileMode.CreateNew,FileAccess.Write,FileShare.None)){file.Write(bytes,0,bytes.Length);file.Flush(true);}
                Report(job,"preparing Waiting for installer...");
                string args="--update-wait "+Quote(root)+" "+pid+" "+started+" "+Quote(job);
                var info=new ProcessStartInfo(setup,args){UseShellExecute=true,Verb=NeedsElevation(root)?"runas":"",WindowStyle=ProcessWindowStyle.Hidden};
                using(var verified=new FileStream(setup,FileMode.Open,FileAccess.Read,FileShare.Read)) {
                    if(!String.Equals(Engine.FileHash(setup),asset.digest.Substring(7),StringComparison.OrdinalIgnoreCase))throw new IOException("Downloaded installer changed.");
                    using(var installer=Process.Start(info)){installer.WaitForExit();if(installer.ExitCode!=0)throw new IOException("Update was not installed. See the update status.");}
                }
                Report(job,"complete Update installed. Starting Skillshot City...");
                Process.Start(new ProcessStartInfo("steam://rungameid/308600"){UseShellExecute=true});
            }catch(Exception error){Report(job,"error "+error.Message);}
        }
        public static void ApplyAfterExit(string root,int pid,long started,string job,Package package) {
            job=Job(job);Engine.VerifyGamePackage(root,package);
            using(var game=Game(root,pid,started)) {
                Report(job,"ready Installer ready; restarting game...");
                if(!game.WaitForExit(120000))throw new IOException("The game stayed open. Update cancelled.");
            }
            Engine.Repair(root,package);
        }
    }
}
