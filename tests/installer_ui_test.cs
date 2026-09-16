using System;
using System.IO;
using System.Text;
using System.Drawing;
using System.Windows.Forms;
using System.Diagnostics;
using System.Threading;
using SSCMods.Setup;
public static class InstallerUiTests {
    static void Check(bool ok,string label){if(!ok)throw new Exception(label);Console.WriteLine("PASS: "+label);}
    static Control Find(Control parent,string name){foreach(Control c in parent.Controls){if(c.Name==name)return c;var found=Find(c,name);if(found!=null)return found;}return null;}
    static void Wait(SetupWindow w){var timer=Stopwatch.StartNew();do{Application.DoEvents();Thread.Sleep(10);if(timer.ElapsedMilliseconds>15000)throw new Exception("UI timed out");}while(w.Busy);Application.DoEvents();}
    static void Click(SetupWindow w,string name){SynchronizationContext.SetSynchronizationContext(new WindowsFormsSynchronizationContext());if(name=="repair"){var link=(LinkLabel)Find(w,name);typeof(LinkLabel).GetMethod("OnLinkClicked",System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic).Invoke(link,new object[]{new LinkLabelLinkClickedEventArgs(link.Links[0])});}else ((Button)Find(w,name)).PerformClick();Wait(w);}
    static void Shot(SetupWindow w,string path){using(var b=new Bitmap(w.Width,w.Height)){w.DrawToBitmap(b,new Rectangle(0,0,b.Width,b.Height));b.Save(path);}}
    [STAThread] public static int Main(string[] args){try{
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        string parent=Path.GetFullPath(args[0]);Directory.CreateDirectory(parent);if(args.Length>1){using(var preview=new SetupWindow(args[1],false)){preview.StartPosition=FormStartPosition.Manual;preview.Location=new Point(-30000,-30000);preview.ShowInTaskbar=false;preview.Show();Wait(preview);Shot(preview,Path.Combine(parent,"setup.png"));preview.Close();}return 0;}string root=Path.Combine(parent,"SkillshotCity");Directory.CreateDirectory(root);
        File.WriteAllText(Path.Combine(root,"SkillshotCity.exe"),"Synthetic UI fixture");File.WriteAllText(Path.Combine(root,"user.txt"),"Keep");
        var p=new Package{RuntimeValidated=true,GameSize=new FileInfo(Path.Combine(root,"SkillshotCity.exe")).Length,GameHash=Engine.FileHash(Path.Combine(root,"SkillshotCity.exe"))};
        foreach(var name in new[]{"opengl32.dll","SSCMods/runtime.dll","SSCMods/Uninstall.exe"})p.Files.Add(new Payload{Path=name,Bytes=Encoding.UTF8.GetBytes("Fixture "+name)});
        using(var w=new SetupWindow(root,false,p)){
            w.StartPosition=FormStartPosition.Manual;w.Location=new Point(-30000,-30000);w.ShowInTaskbar=false;w.Show();Wait(w);
            Check(Find(w,"action").Enabled&&Find(w,"action").Text=="INSTALL","clean folder selects Install");Shot(w,Path.Combine(parent,"install.png"));
            Check(!Find(w,"remove").Enabled&&Find(w,"repair").Visible&&Find(w,"repair").Enabled,"Repair available without mod; Uninstall unavailable");
            Click(w,"action");Check(File.Exists(Path.Combine(root,Engine.ManifestName)),"Install button writes package: "+Find(w,"status").Text);
            Check(Find(w,"action").Text=="LAUNCH","Install offers Launch");SynchronizationContext.SetSynchronizationContext(new WindowsFormsSynchronizationContext());w.RefreshInspection();Wait(w);Check(Find(w,"action").Enabled&&Find(w,"action").Text=="UPDATE","installed folder selects Update");Shot(w,Path.Combine(parent,"update.png"));
            Click(w,"action");Check(Find(w,"status").Text=="SSC Mod Menu is ready.","Update completes with success state");
            Check(Find(w,"remove").Enabled,"same installer offers Uninstall");Shot(w,Path.Combine(parent,"uninstall.png"));
            File.WriteAllText(Path.Combine(root,"opengl32.dll"),"Edited");Click(w,"remove");Check(File.Exists(Path.Combine(root,"opengl32.dll"))&&Find(w,"status").Text=="Some changed files were kept.","Uninstall preserves changed files and explains partial result");
            File.WriteAllBytes(Path.Combine(root,"opengl32.dll"),p.Files[0].Bytes);SynchronizationContext.SetSynchronizationContext(new WindowsFormsSynchronizationContext());w.RefreshInspection();Wait(w);Click(w,"remove");
            Check(!File.Exists(Path.Combine(root,Engine.ManifestName))&&File.ReadAllText(Path.Combine(root,"user.txt"))=="Keep","Uninstall removes only owned files");Shot(w,Path.Combine(parent,"complete.png"));
            Click(w,"repair");Check(File.Exists(Path.Combine(root,Engine.ManifestName)),"Repair installs when missing");
            File.Delete(Path.Combine(root,"SSCMods/runtime.dll"));SynchronizationContext.SetSynchronizationContext(new WindowsFormsSynchronizationContext());w.RefreshInspection();Wait(w);
            Check(!Find(w,"action").Enabled&&Find(w,"repair").Enabled,"Repair remains available for damaged install");
            Click(w,"repair");Check(Engine.FileHash(Path.Combine(root,"SSCMods/runtime.dll"))==Engine.Hash(p.Files[1].Bytes),"Repair button restores missing file");
            File.WriteAllText(Path.Combine(root,"SkillshotCity.exe"),"Unknown revision");SynchronizationContext.SetSynchronizationContext(new WindowsFormsSynchronizationContext());w.RefreshInspection();Wait(w);Check(!Find(w,"action").Enabled,"unknown revision disables primary action");Shot(w,Path.Combine(parent,"unsupported.png"));
            w.Close();
        }
        Console.WriteLine("PASS: themed installer workflow and render checks");return 0;
    }catch(Exception e){Console.Error.WriteLine(e);return 1;}}
}




