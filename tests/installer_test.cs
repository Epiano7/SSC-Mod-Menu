using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Web.Script.Serialization;
using System.Drawing;
using SSCMods.Setup;

public static class InstallerTests {
    static int passed;
    static void Check(bool ok,string label) { if(!ok) throw new Exception("FAIL: "+label); passed++; Console.WriteLine("PASS: "+label); }
    static void Reject(Action action,string label) { bool rejected=false; try {action();} catch(IOException) {rejected=true;} Check(rejected,label); }
    static string Fixture(string parent) {
        string root=Path.Combine(parent,Guid.NewGuid().ToString("N")); Directory.CreateDirectory(root);
        File.WriteAllText(Path.Combine(root,"SkillshotCity.exe"),"Synthetic game fingerprint fixture; not a game binary.");
        File.WriteAllText(Path.Combine(root,"user-file.txt"),"preserve me"); return root;
    }
    static Package PackageFor(string root) {
        var p=new Package {RuntimeValidated=true,GameSize=new FileInfo(Path.Combine(root,"SkillshotCity.exe")).Length,GameHash=Engine.FileHash(Path.Combine(root,"SkillshotCity.exe"))};
        foreach(string name in new[]{"opengl32.dll","SSCMods/runtime.dll","SSCMods/Uninstall.exe"})
            p.Files.Add(new Payload {Path=name,Bytes=Encoding.UTF8.GetBytes("synthetic owned fixture: "+name)});
        return p;
    }
    [STAThread]
    public static int Main(string[] args) {
        try {
            string parent=Path.GetFullPath(args[0]); Directory.CreateDirectory(parent);
            string root=Fixture(parent); var package=PackageFor(root);
            Check(!Engine.Inspect(root,Release.Package()).CanInstall,"production preview refuses unknown game");
            Reject(()=>Engine.Install(root,Release.Package()),"production preview cannot install fixture");
            Check(!Directory.Exists(Path.Combine(root,"SSCMods")),"refused installation leaves no mod folder");
            Engine.Install(root,package);
            Check(File.Exists(Path.Combine(root,Engine.ManifestName)),"installation writes manifest");
            Check(package.Files.All(f=>Engine.FileHash(Path.Combine(root,f.Path))==Engine.Hash(f.Bytes)),"installed payload hashes match");
            Reject(()=>Engine.Install(root,package),"repeat install cannot overwrite existing mod");
            Check(Engine.Uninstall(root).Count==0,"clean uninstall completes");
            Check(!Directory.Exists(Path.Combine(root,"SSCMods"))&&!File.Exists(Path.Combine(root,"opengl32.dll")),"uninstall removes owned additions");
            Check(Engine.FileHash(Path.Combine(root,"SkillshotCity.exe"))==package.GameHash && File.ReadAllText(Path.Combine(root,"user-file.txt"))=="preserve me","game and unrelated file unchanged");
            foreach(int failAt in new[]{0,1,2,3}) {
                string upgrade=Fixture(parent);var oldPackage=PackageFor(upgrade);Engine.Install(upgrade,oldPackage);
                File.WriteAllText(Path.Combine(upgrade,"SSCMods","personal.txt"),"keep");
                File.WriteAllText(Path.Combine(upgrade,"SkillshotCity.exe"),"Synthetic newer Steam game revision.");
                Reject(()=>Engine.InspectUpdate(upgrade,oldPackage),"old package refuses newer game revision "+failAt);
                var next=PackageFor(upgrade);foreach(var payload in next.Files) payload.Bytes=Encoding.UTF8.GetBytes("new version: "+payload.Path);
                Check(Engine.InspectUpdate(upgrade,next).CanInstall,"owned installation upgrade preflight "+failAt);
                if(failAt==0) Engine.Update(upgrade,next);
                else Reject(()=>Engine.Update(upgrade,next,n=>{if(n==failAt) throw new IOException("Injected replacement failure");}),"upgrade failure "+failAt);
                var expected=failAt==0?next:oldPackage;
                Check(expected.Files.All(f=>Engine.FileHash(Path.Combine(upgrade,f.Path))==Engine.Hash(f.Bytes)),"upgrade or rollback payload integrity "+failAt);
                Check(Engine.InspectUpdate(upgrade,next).CanInstall,"upgrade or rollback manifest integrity "+failAt);
                var savedManifest=new JavaScriptSerializer().Deserialize<Manifest>(File.ReadAllText(Path.Combine(upgrade,Engine.ManifestName)));
                Check(savedManifest.GameSha256==expected.GameHash,"game-revision manifest upgrade or rollback "+failAt);
                Check(File.ReadAllText(Path.Combine(upgrade,"SSCMods","personal.txt"))=="keep"&&Engine.FileHash(Path.Combine(upgrade,"SkillshotCity.exe"))==next.GameHash,"upgrade preserves updated game and unrelated files "+failAt);
                Check(Engine.Uninstall(upgrade).Count==0,"uninstall after upgrade or rollback "+failAt);
            }
            string changedUpgrade=Fixture(parent);var changedPackage=PackageFor(changedUpgrade);Engine.Install(changedUpgrade,changedPackage);
            File.WriteAllText(Path.Combine(changedUpgrade,"SSCMods","runtime.dll"),"modified");
            Reject(()=>Engine.Update(changedUpgrade,changedPackage),"update refuses modified owned file");
            Check(File.ReadAllText(Path.Combine(changedUpgrade,"SSCMods","runtime.dll"))=="modified","update preserves modified owned file");
            Engine.Install(root,package);
            File.WriteAllText(Path.Combine(root,"opengl32.dll"),"user modified");
            File.WriteAllText(Path.Combine(root,"SSCMods","notes.txt"),"personal notes");
            Check(Engine.Uninstall(root).SequenceEqual(new[]{"opengl32.dll"}),"modified proxy preserved");
            Check(File.Exists(Path.Combine(root,Engine.ManifestName))&&File.Exists(Path.Combine(root,"SSCMods","notes.txt")),"recovery manifest and unrelated mod-folder file preserved");
            foreach(int failAt in new[]{1,2,3}) {
                string rollback=Fixture(parent); var p=PackageFor(rollback);
                Reject(()=>Engine.Install(rollback,p,n=>{if(n==failAt) throw new IOException("Injected copy failure");}),"injected failure after file "+failAt);
                Check(!Directory.Exists(Path.Combine(rollback,"SSCMods"))&&!File.Exists(Path.Combine(rollback,"opengl32.dll")),"rollback after file "+failAt);
            }
            string conflict=Fixture(parent); File.WriteAllText(Path.Combine(conflict,"opengl32.dll"),"existing SSCVR or another mod");
            Reject(()=>Engine.Install(conflict,PackageFor(conflict)),"existing proxy conflict rejected");
            Check(File.ReadAllText(Path.Combine(conflict,"opengl32.dll"))=="existing SSCVR or another mod","existing proxy preserved");
            string malformed=Fixture(parent); Engine.Install(malformed,PackageFor(malformed));
            string manifestPath=Path.Combine(malformed,Engine.ManifestName);
            var json=new JavaScriptSerializer(); var m=json.Deserialize<Manifest>(File.ReadAllText(manifestPath));
            m.Files[0].Path="SkillshotCity.exe"; File.WriteAllText(manifestPath,json.Serialize(m));
            Reject(()=>Engine.Uninstall(malformed),"manifest cannot target game executable");
            Check(File.Exists(Path.Combine(malformed,"SSCMods","runtime.dll")),"entire manifest validated before removal");
            m.Files[0].Path="../outside.txt"; File.WriteAllText(manifestPath,json.Serialize(m));
            Reject(()=>Engine.Uninstall(malformed),"manifest traversal rejected");
            string incomplete=Fixture(parent); var missing=PackageFor(incomplete); missing.Files.RemoveAt(0);
            Reject(()=>Engine.Install(incomplete,missing),"incomplete runtime payload rejected");
            string mismatch=Fixture(parent); var wrong=PackageFor(mismatch); wrong.GameHash=new string('0',64);
            Reject(()=>Engine.Install(mismatch,wrong),"mismatched exact build rejected");
            string foreign=Fixture(parent);
            Directory.CreateDirectory(Path.Combine(foreign,"SSCMods"));
            Reject(()=>Engine.Install(foreign,PackageFor(foreign)),"existing SSCMods folder rejected");
            string interrupted=Fixture(parent); Engine.Install(interrupted,PackageFor(interrupted));
            string journal=Path.Combine(interrupted,Engine.ManifestName);
            var partial=json.Deserialize<Manifest>(File.ReadAllText(journal)); partial.State="Installing";
            File.WriteAllText(journal,json.Serialize(partial));
            Check(Engine.Uninstall(interrupted).Count==0,"interrupted-install journal supports recovery");
            string edited=Fixture(parent); var editedPackage=PackageFor(edited);
            Reject(()=>Engine.Install(edited,editedPackage,n=>{if(n==1) {File.WriteAllText(Path.Combine(edited,"opengl32.dll"),"changed during install"); throw new IOException("Injected failure");}}),"failure with modified payload");
            Check(File.ReadAllText(Path.Combine(edited,"opengl32.dll"))=="changed during install"&&File.Exists(Path.Combine(edited,Engine.ManifestName)),"rollback preserves changed payload and recovery journal");
            if(args.Length>1) {
                var actual=Engine.Inspect(args[1],Release.Package());
                Check(actual.CanInstall==Release.Package().RuntimeValidated,"actual build preflight follows release validation gate");
                Console.WriteLine("Steam preflight: "+actual.Size+" bytes; SHA256="+actual.Hash+"; "+actual.Message);
                Check(Engine.Discover().Equals(actual.Root,StringComparison.OrdinalIgnoreCase),"Steam registry/library discovery");
                using(var window=new SetupWindow(actual.Root,false)) {
                    window.StartPosition=System.Windows.Forms.FormStartPosition.Manual;
                    window.Location=new Point(-30000,-30000); window.ShowInTaskbar=false;
                    window.Show(); System.Windows.Forms.Application.DoEvents();
                    window.RefreshInspection();
                    using(var bitmap=new Bitmap(window.Width,window.Height)) {
                        window.DrawToBitmap(bitmap,new Rectangle(0,0,bitmap.Width,bitmap.Height));
                        bitmap.Save(Path.Combine(parent,"setup-window.png"));
                    }
                    window.Close();
                }
            }
            Console.WriteLine("All "+passed+" installer checks passed. Fixtures retained under "+parent); return 0;
        } catch(Exception error) {Console.Error.WriteLine(error); return 1;}
    }
}
