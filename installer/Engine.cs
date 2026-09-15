using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;
using Microsoft.Win32;

namespace SSCMods.Setup {
    public sealed class OwnedFile {
        public string Path { get; set; }
        public string Sha256 { get; set; }
    }
    public sealed class Manifest {
        public int Schema { get; set; }
        public string Product { get; set; }
        public string State { get; set; }
        public string GameSha256 { get; set; }
        public List<OwnedFile> Files { get; set; }
    }
    public sealed class Payload {
        public string Path;
        public byte[] Bytes;
    }
    // Populated only by a release that has passed isolated runtime validation.
    // There is deliberately no command-line switch to override this release gate.
    public sealed class Package {
        public bool RuntimeValidated;
        public long GameSize;
        public string GameHash;
        public List<Payload> Files = new List<Payload>();
    }
    public sealed class Inspection {
        public string Root, Hash, Message;
        public long Size;
        public bool CanInstall;
    }
    public static class Engine {
        public const string Product = "SSCMods";
        public const string ManifestName = "SSCMods/install-manifest.json";
        static readonly string[] Allowed = { "opengl32.dll", "SSCMods/runtime.dll", "SSCMods/Uninstall.exe" };
        static readonly JavaScriptSerializer Json = new JavaScriptSerializer();
        [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern bool CreateDirectory(string path, IntPtr attributes);

        public static string Hash(byte[] data) {
            using (var sha=SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(data)).Replace("-", "");
        }
        public static string FileHash(string path) {
            using (var stream=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read))
            using (var sha=SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "");
        }
        public static void RejectLinks(string path) {
            for (string current=System.IO.Path.GetFullPath(path); !String.IsNullOrEmpty(current); current=System.IO.Path.GetDirectoryName(current)) {
                // GetAttributes also catches dangling links, unlike File.Exists.
                try {
                    if ((File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)
                        throw new IOException("Linked paths are not supported: "+current);
                } catch (FileNotFoundException) {} catch (DirectoryNotFoundException) {}
            }
        }
        public static string Root(string path) {
            string root=System.IO.Path.GetFullPath(path).TrimEnd('\\','/');
            if (root.StartsWith(@"\\") || !Directory.Exists(root) || root.Length<4)
                throw new IOException("Choose an existing local game directory.");
            RejectLinks(root);
            return root;
        }
        static string Destination(string root,string relative) {
            if (!Allowed.Contains(relative) && relative!=ManifestName)
                throw new IOException("Manifest contains an unrecognized destination.");
            string result=System.IO.Path.Combine(root,relative.Replace('/','\\'));
            RejectLinks(result);
            return result;
        }
        public static void RequireGameClosed() {
            foreach (var process in Process.GetProcessesByName("SkillshotCity")) {
                process.Dispose();
                throw new IOException("Close Skillshot City before installing or removing the mod.");
            }
        }
        public static string Discover() {
            var libraries=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            using (var key=Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam"))
                AddRegistry(libraries,key);
            using (var key=Registry.LocalMachine.OpenSubKey(@"SOFTWARE\WOW6432Node\Valve\Steam"))
                AddRegistry(libraries,key);
            foreach (string root in libraries.ToArray()) {
                string vdf=System.IO.Path.Combine(root,@"steamapps\libraryfolders.vdf");
                if (!File.Exists(vdf)) continue;
                foreach (Match m in Regex.Matches(File.ReadAllText(vdf),"\"path\"\\s+\"([^\"]+)\""))
                    libraries.Add(m.Groups[1].Value.Replace(@"\\",@"\"));
            }
            foreach (string library in libraries) {
                string manifest=System.IO.Path.Combine(library,@"steamapps\appmanifest_308600.acf");
                if (!File.Exists(manifest)) continue;
                string acf=File.ReadAllText(manifest);
                if (!Regex.IsMatch(acf,"\"appid\"\\s+\"308600\"")) continue;
                Match m=Regex.Match(acf,"\"installdir\"\\s+\"([^\"]+)\"");
                string dir=m.Groups[1].Value;
                if (!m.Success || dir=="." || dir==".." || dir.IndexOfAny(new[]{'\\','/',':'})>=0) continue;
                string candidate=System.IO.Path.Combine(library,"steamapps","common",dir);
                if (File.Exists(System.IO.Path.Combine(candidate,"SkillshotCity.exe"))) return Root(candidate);
            }
            throw new IOException("Steam app 308600 was not found. Select its installed game folder.");
        }
        static void AddRegistry(HashSet<string> roots,RegistryKey key) {
            if (key==null) return;
            foreach (string name in new[]{"SteamPath","InstallPath"}) {
                string value=key.GetValue(name) as string;
                if (!String.IsNullOrWhiteSpace(value)) roots.Add(value);
            }
        }
        static void ValidatePackage(Package package) {
            if (!package.RuntimeValidated) throw new IOException("This preview has no validated game runtime. Installation is unavailable; no game files were changed.");
            if (package.GameSize<=0 || !Regex.IsMatch(package.GameHash??"","^[A-F0-9]{64}$")) throw new IOException("Invalid release build metadata.");
            if (package.Files.Count!=Allowed.Length || !Allowed.All(p=>package.Files.Count(f=>f.Path==p)==1) ||
                package.Files.Any(f=>f.Bytes==null || f.Bytes.Length==0)) throw new IOException("Incomplete embedded release payload.");
        }
        public static Inspection Inspect(string path,Package package) {
            string root=Root(path), exe=System.IO.Path.Combine(root,"SkillshotCity.exe");
            RejectLinks(exe);
            var result=new Inspection { Root=root,Size=new FileInfo(exe).Length,Hash=FileHash(exe) };
            bool known=(result.Size==15222272 && result.Hash=="7E8BE56E148D9E5C8A4D5B38A7A2D7C4352FFE70EAE7CEF56111CFF48BFCDA58") ||
                       (result.Size==15221248 && result.Hash=="487962A3A057A7DF6574DB98536518985AF18DA6A66C145FC23F6AC35F54D154");
            if (package.RuntimeValidated) known=result.Size==package.GameSize && result.Hash==package.GameHash;
            if (!known) { result.Message="Unknown executable revision. Installation blocked."; return result; }
            RequireGameClosed();
            foreach(string relative in new[]{"opengl32.dll","SSCMods"}) {
                string target=System.IO.Path.Combine(root,relative); RejectLinks(target);
                if(File.Exists(target)||Directory.Exists(target)) {
                    result.Message="Existing "+relative+" detected. Nothing will be overwritten. Remove the existing mod using its own uninstaller first."; return result;
                }
            }
            try { ValidatePackage(package); }
            catch(IOException error) { result.Message=error.Message; return result; }
            result.CanInstall=true; result.Message="Ready. Only the files listed below will be added."; return result;
        }
        static void WriteNew(string path,byte[] bytes) {
            using(var stream=new FileStream(path,FileMode.CreateNew,FileAccess.Write,FileShare.None)) {
                stream.Write(bytes,0,bytes.Length); stream.Flush(true);
            }
        }
        static void WriteManifest(string root,Manifest manifest) {
            string path=Destination(root,ManifestName);
            // A complete JSON document is written atomically; previous journal survives interruption.
            string temp=System.IO.Path.Combine(root,"SSCMods",Guid.NewGuid().ToString("N")+".tmp");
            try {
                WriteNew(temp,Encoding.UTF8.GetBytes(Json.Serialize(manifest)));
                if (File.Exists(path)) File.Replace(temp,path,null); else File.Move(temp,path);
            } finally { if(File.Exists(temp)) File.Delete(temp); }
        }
        public static void Install(string path,Package package,Action<int> afterCopy=null) {
            Inspection check=Inspect(path,package);
            if(!check.CanInstall) throw new IOException(check.Message);
            string root=check.Root, ownedDir=System.IO.Path.Combine(root,"SSCMods");
            if(!CreateDirectory(ownedDir,IntPtr.Zero)) throw new IOException("Could not create a new SSCMods folder. No existing folder will be reused.");
            var manifest=new Manifest {Schema=1,Product=Product,State="Installing",GameSha256=check.Hash,Files=new List<OwnedFile>()};
            var created=new List<OwnedFile>();
            try {
                WriteManifest(root,manifest);
                int count=0;
                foreach(var file in package.Files) {
                    RequireGameClosed();
                    string destination=Destination(root,file.Path);
                    var record=new OwnedFile {Path=file.Path,Sha256=Hash(file.Bytes)};
                    // Journal intent first. Recovery will remove a file only if its hash matches.
                    manifest.Files.Add(record); WriteManifest(root,manifest);
                    bool opened=false;
                    try {
                        using(var stream=new FileStream(destination,FileMode.CreateNew,FileAccess.Write,FileShare.None)) {
                            opened=true; created.Add(record);
                            stream.Write(file.Bytes,0,file.Bytes.Length); stream.Flush(true);
                        }
                    }
                    catch {
                        // Do not claim an existing file if CreateNew failed.
                        if(!opened) { manifest.Files.Remove(record); WriteManifest(root,manifest); }
                        throw;
                    }
                    if(FileHash(destination)!=record.Sha256) throw new IOException("Installed payload verification failed.");
                    if(afterCopy!=null) afterCopy(++count);
                }
                manifest.State="Installed"; WriteManifest(root,manifest);
            } catch {
                foreach(var record in created.AsEnumerable().Reverse()) {
                    string destination=Destination(root,record.Path);
                    if(File.Exists(destination)&&FileHash(destination)==record.Sha256) File.Delete(destination);
                }
                // Preserve the journal when a changed/partial file requires manual recovery.
                bool remaining=manifest.Files.Any(f=>File.Exists(Destination(root,f.Path)));
                if(!remaining) {
                    string journal=Destination(root,ManifestName);
                    if(File.Exists(journal)) File.Delete(journal);
                    if(!Directory.EnumerateFileSystemEntries(ownedDir).Any()) Directory.Delete(ownedDir);
                }
                throw;
            }
        }
        public static Inspection InspectUpdate(string path,Package package) {
            ValidatePackage(package); RequireGameClosed();
            string root=Root(path),exe=Path.Combine(root,"SkillshotCity.exe"); RejectLinks(exe);
            var result=new Inspection {Root=root,Size=new FileInfo(exe).Length,Hash=FileHash(exe)};
            if(result.Size!=package.GameSize||result.Hash!=package.GameHash) throw new IOException("Unknown executable revision. Update blocked.");
            var manifest=ReadManifest(root);
            // Steam may replace the game between mod releases. The new package must
            // match the current executable; the old manifest establishes file ownership.
            if(manifest.State!="Installed"||!Regex.IsMatch(manifest.GameSha256??"","^[A-F0-9]{64}$")||manifest.Files.Count!=Allowed.Length)
                throw new IOException("Incomplete or incompatible installation manifest. Update blocked.");
            foreach(var file in manifest.Files) {
                string target=Destination(root,file.Path);
                if(!File.Exists(target)||FileHash(target)!=file.Sha256) throw new IOException("Modified or missing mod file: "+file.Path+". Update blocked.");
            }
            result.CanInstall=true;result.Message="Ready to update SSC Mods. Saved settings and game files are preserved.";return result;
        }
        public static void Update(string path,Package package,Action<int> afterCopy=null) {
            var check=InspectUpdate(path,package);string root=check.Root;
            var previous=ReadManifest(root);
            string backup=Path.Combine(root,"SSCMods","update-backup-"+Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(backup);
            var replaced=new List<Payload>();
            // Durable backups remain after an interrupted process for manual recovery.
            File.Copy(Destination(root,ManifestName),Path.Combine(backup,"manifest.json"),false);
            try {
                foreach(var file in package.Files) {
                    string staged=Path.Combine(backup,Path.GetFileName(file.Path)+".new");
                    WriteNew(staged,file.Bytes);
                    if(FileHash(staged)!=Hash(file.Bytes)) throw new IOException("Staged payload verification failed.");
                }
                int count=0;
                foreach(var file in package.Files) {
                    RequireGameClosed();string target=Destination(root,file.Path);
                    var old=previous.Files.Single(f=>f.Path==file.Path);
                    if(FileHash(target)!=old.Sha256) throw new IOException("Mod file changed during update.");
                    File.Replace(Path.Combine(backup,Path.GetFileName(file.Path)+".new"),target,Path.Combine(backup,Path.GetFileName(file.Path)+".old"));
                    replaced.Add(file);
                    if(FileHash(target)!=Hash(file.Bytes)) throw new IOException("Updated payload verification failed.");
                    if(afterCopy!=null) afterCopy(++count);
                }
                WriteManifest(root,new Manifest {Schema=1,Product=Product,State="Installed",GameSha256=check.Hash,
                    Files=package.Files.Select(f=>new OwnedFile {Path=f.Path,Sha256=Hash(f.Bytes)}).ToList()});
            } catch(Exception error) {
                bool restored=true;
                foreach(var file in replaced.AsEnumerable().Reverse()) {
                    string target=Destination(root,file.Path);
                    try {
                        if(FileHash(target)!=Hash(file.Bytes)) {restored=false;continue;}
                        File.Replace(Path.Combine(backup,Path.GetFileName(file.Path)+".old"),target,null);
                    } catch {restored=false;}
                }
                if(restored) WriteManifest(root,previous);
                throw new IOException((restored?"Update rolled back. ":"Update incomplete. ")+"Recovery files: "+backup+". "+error.Message,error);
            }
            // Delete only files this transaction created; never recurse into an existing directory.
            foreach(var file in package.Files) File.Delete(Path.Combine(backup,Path.GetFileName(file.Path)+".old"));
            File.Delete(Path.Combine(backup,"manifest.json"));Directory.Delete(backup);
        }
        public static void InstallOrUpdate(string path,Package package) {
            string root=Root(path);
            if(File.Exists(Destination(root,ManifestName))) Update(root,package);else Install(root,package);
        }
        static Manifest ReadManifest(string root) {
            string path=Destination(root,ManifestName);
            var info=new FileInfo(path);
            if(!info.Exists || info.Length>65536) throw new IOException("Valid installation manifest not found.");
            Manifest m=Json.Deserialize<Manifest>(File.ReadAllText(path));
            if(m==null || m.Schema!=1 || m.Product!=Product || (m.State!="Installed" && m.State!="Installing") ||
                m.Files==null || m.Files.Count>Allowed.Length || m.Files.Select(f=>f.Path).Distinct().Count()!=m.Files.Count)
                throw new IOException("Invalid installation manifest.");
            foreach(var file in m.Files) {
                Destination(root,file.Path);
                if(!Regex.IsMatch(file.Sha256??"","^[A-F0-9]{64}$")) throw new IOException("Invalid owned-file hash.");
            }
            return m;
        }
        public static Inspection InspectRemoval(string path) {
            string root=Root(path); RequireGameClosed(); ReadManifest(root);
            return new Inspection {Root=root,CanInstall=true,Message="Ready to uninstall."};
        }
        public static List<string> Uninstall(string path) {
            string root=Root(path); RequireGameClosed();
            Manifest manifest=ReadManifest(root); // Validate every path before deleting anything.
            var preserve=new List<string>();
            foreach(var file in manifest.Files) {
                string destination=Destination(root,file.Path);
                if(!File.Exists(destination)) {
                    if(Directory.Exists(destination)) preserve.Add(file.Path);
                    continue;
                }
                // Hold a non-write-sharing handle while comparing bytes and marking deletion.
                using(var stream=new FileStream(destination,FileMode.Open,FileAccess.Read,FileShare.Read|FileShare.Delete)) {
                    string hash;
                    using(var sha=SHA256.Create()) hash=BitConverter.ToString(sha.ComputeHash(stream)).Replace("-","");
                    if(hash!=file.Sha256) { preserve.Add(file.Path); continue; }
                    File.Delete(destination);
                }
            }
            if(preserve.Count==0) {
                File.Delete(Destination(root,ManifestName));
                string dir=System.IO.Path.Combine(root,"SSCMods");
                if(!Directory.EnumerateFileSystemEntries(dir).Any()) Directory.Delete(dir);
            }
            return preserve;
        }
    }
}
