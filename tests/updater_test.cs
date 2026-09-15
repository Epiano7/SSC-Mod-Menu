using System;
using System.IO;
using System.Text;
using SSCMods.Setup;
using System.Diagnostics;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;
public static class UpdaterTests {
 static int count;
 static void Check(bool ok,string label){if(!ok)throw new Exception(label);++count;Console.WriteLine("PASS: "+label);}
 static void Reject(Action action,string label){try{action();}catch(Exception){Check(true,label);return;}throw new Exception(label);}
 static string Metadata(string tag,string digest,bool draft=false,bool preview=false){return "{\"tag_name\":\""+tag+"\",\"draft\":"+draft.ToString().ToLower()+",\"prerelease\":"+preview.ToString().ToLower()+",\"assets\":[{\"name\":\"SSC-Mod-Menu-Setup.exe\",\"state\":\"uploaded\",\"size\":1024,\"digest\":\""+digest+"\",\"browser_download_url\":\"https://github.com/Epiano7/SSC-Mod-Menu/releases/download/"+tag+"/SSC-Mod-Menu-Setup.exe\"}]}";}
 public static int Main(string[] args){try{
  if(args.Length==2&&args[0]=="--fixture"){for(int i=0;i<2000&&!File.Exists(args[1]);++i)Thread.Sleep(10);return 0;}
  var data=new byte[1024];string digest="sha256:"+Engine.Hash(data).ToLower(),version;var json=Metadata("v0.2.0",digest);
  var asset=Updater.Select(json,"0.1.0",out version);Check(asset!=null&&version=="0.2.0","new stable release selected");Updater.Verify(data,asset);Check(true,"asset size and hash verified");
  Check(Updater.Select(json,"0.2.0",out version)==null,"equal version ignored");
  Check(Updater.Select(json,"0.3.0",out version)==null,"downgrade ignored");
  Check(Updater.Select(Metadata("v9.0.0",digest,true),"0.1.0",out version)==null,"draft ignored");
  Check(Updater.Select(Metadata("v9.0.0",digest,false,true),"0.1.0",out version)==null,"prerelease ignored");
  Reject(()=>Updater.Select(Metadata("v9.0.0", ""),"0.1.0",out version),"missing checksum rejected");
  Reject(()=>Updater.Select(json.Replace("https://github.com/","https://example.com/"),"0.1.0",out version),"foreign download rejected");
  Reject(()=>Updater.Select(json.Replace("Epiano7/SSC-Mod-Menu","someone/SSC-Mod-Menu"),"0.1.0",out version),"foreign repository rejected");
  Reject(()=>Updater.Select(json.Replace("1024","200000000"),"0.1.0",out version),"oversized installer rejected");
  Reject(()=>Updater.Select(json.Replace("uploaded","new"),"0.1.0",out version),"unfinished asset rejected");
  Reject(()=>Updater.Select(json.Replace("v0.2.0","v0.2.0/../../x"),"0.1.0",out version),"invalid tag rejected");
  Reject(()=>Updater.Verify(new byte[10],asset),"truncated download rejected");data[0]=1;Reject(()=>Updater.Verify(data,asset),"corrupt download rejected");
  Reject(()=>Updater.Quote("bad\"arg"),"argument quote injection rejected");
  string job=Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),"handoff-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(job);
  string gameExe=Path.Combine(job,"SkillshotCity.exe");File.Copy(Assembly.GetExecutingAssembly().Location,gameExe);
  var package=new Package{RuntimeValidated=true,GameSize=new FileInfo(gameExe).Length,GameHash=Engine.FileHash(gameExe)};
  foreach(string name in new[]{"opengl32.dll","SSCMods/runtime.dll","SSCMods/Uninstall.exe"})package.Files.Add(new Payload{Path=name,Bytes=Encoding.UTF8.GetBytes("original "+name)});
  Engine.Install(job,package);byte[] original=package.Files[1].Bytes;
  foreach(var payload in package.Files)payload.Bytes=Encoding.UTF8.GetBytes("updated "+payload.Path);
  string exit=Path.Combine(job,"exit");
  using(var child=Process.Start(new ProcessStartInfo(gameExe,"--fixture "+Updater.Quote(exit)){UseShellExecute=false,CreateNoWindow=true})){
   long began=child.StartTime.ToUniversalTime().Ticks;
   Reject(()=>Updater.Game(job,child.Id,began+1),"reused or mismatched process identity rejected");
   var apply=Task.Run(()=>Updater.ApplyAfterExit(job,child.Id,began,job,package));
   var timer=Stopwatch.StartNew();while(!File.Exists(Path.Combine(job,"status.txt"))&&!apply.IsCompleted&&timer.ElapsedMilliseconds<10000)Thread.Sleep(10);
   Check(File.ReadAllText(Path.Combine(job,"status.txt")).StartsWith("ready "),"installer signals readiness before game shutdown");
   Check(Engine.FileHash(Path.Combine(job,"SSCMods/runtime.dll"))==Engine.Hash(original),"running game files stay unchanged");
   File.WriteAllText(exit,"");Check(apply.Wait(10000),"handoff completes after game exit");
   Check(Engine.FileHash(Path.Combine(job,"SSCMods/runtime.dll"))==Engine.Hash(package.Files[1].Bytes),"silent handoff installs verified payload");
  }
  Console.WriteLine("All "+count+" updater checks passed.");return 0;
 }catch(Exception e){Console.Error.WriteLine(e);return 1;}}
}
