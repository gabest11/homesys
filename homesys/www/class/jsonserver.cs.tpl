using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Runtime.CompilerServices;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Homesys.Data
{
	// Install-Package Microsoft.Net.Http
	// Install-Package Microsoft.Bcl.Async

	[DataContract]
	public class Result<T>
	{
		[DataMember]
		public T Obj;

		[DataMember]
		public string Error;

		[DataMember]
		public int ErrorCode;
	}

	public class ServiceException : Exception
	{
		readonly int _code;

		public int Code { get { return _code; } }

		public ServiceException()
		{
			_code = -1;
		}

		public ServiceException(string message, int code)
			: base(message)
		{
			_code = code;
		}

		public ServiceException(string message, int code, Exception inner)
			: base(message, inner)
		{
			_code = code;
		}
	}

	public class ServiceParameter
	{
		public Uri ServiceUrl;
		public Dictionary<string, object> Args = new Dictionary<string, object>();
		public Dictionary<string, object> Headers = new Dictionary<string, object>();
	}

	public class ServiceEventArgs : EventArgs
	{
		string _method;

		public string Method { get { return _method; } }

		ServiceParameter _param;

		public ServiceParameter Parameter { get { return _param; } }

		internal object _result;

		public object Result { get { return _result; } }

		public ServiceEventArgs(string method, ServiceParameter param, object result)
		{
			_method = method;
			_param = param;
			_result = result;
		}
	}

	[DataContract]
	public partial class ServiceNotifyPropertyChanged : INotifyPropertyChanged
	{
		public event PropertyChangedEventHandler PropertyChanged;

		protected bool SetProperty<T>(ref T storage, T value, [CallerMemberName] string propertyName = null)
		{
			if(Equals(storage, value))
			{
				return false;
			}

			storage = value;

			this.OnPropertyChanged(propertyName);

			return true;
		}

		protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
		{
			PropertyChangedEventHandler eventHandler = this.PropertyChanged;

			if(eventHandler != null)
			{
				eventHandler(this, new PropertyChangedEventArgs(propertyName));
			}
		}
	}

	public partial class Service
	{
		static CookieContainer _cookies = new CookieContainer();

		public static CookieContainer Cookies
		{
			get { return _cookies; }
		}

		private string _sessionId;

		public string SessionId 
		{
			get { return _sessionId; }
			set { _sessionId = value; }
		}

		private static Uri _svcUrlBase = new Uri("%URL%");

		public static Uri ServiceUrlBase 
		{
			get { return _svcUrlBase; }
		}

		private Uri _svcUrl;

		public Uri ServiceUrl 
		{
			get { return _svcUrl; }
		}

		public delegate void ServiceEventHandler(Service sender, ServiceEventArgs e);

		public event ServiceEventHandler Calling;
		public event ServiceEventHandler Completed;

		public Service(Uri svcUrl = null)
		{
			_svcUrl = svcUrl ?? _svcUrlBase;
		}

		public async Task<T> Call<T>(Dictionary<string, object> args, CancellationToken? ct = null, ServiceParameter param = null)
		{
			if(param == null)
			{
				param = new ServiceParameter();

				param.ServiceUrl = ServiceUrl;
			}

			object method;

			if(!args.TryGetValue("method", out method))
			{
				method = "?";
			}

			var eventArgs = new ServiceEventArgs((string)method, param, null);

			if(Calling != null)
			{
				Calling(this, eventArgs);
			}

			StringBuilder sb = new StringBuilder();

			if(param.Args != null)
			{
				foreach(var arg in param.Args)
				{
					args.Add(arg.Key, arg.Value);
				}
			}

			foreach(var arg in args)
			{
				if(arg.Value != null)
				{
					sb.Append(arg.Key + "=" + Uri.EscapeDataString(arg.Value.ToString()) + "&");
				}
			}

			if(_sessionId != null && !args.ContainsKey("sessionId"))
			{
				sb.Append("sessionId=" + Uri.EscapeDataString(_sessionId) + "&");
			}

			sb.Append("__DOTNET__=1");

			using(HttpClientHandler handler = new HttpClientHandler())
			{
				handler.CookieContainer = _cookies;

				using(HttpClient client = new HttpClient(handler))
				{
					if(handler.SupportsAutomaticDecompression)
					{
						handler.AutomaticDecompression = DecompressionMethods.GZip | DecompressionMethods.Deflate;
					}

					using(HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Post, param.ServiceUrl))
					{
						request.Content = new StringContent(sb.ToString(), Encoding.UTF8, "application/x-www-form-urlencoded");

						request.Headers.AcceptEncoding.Add(System.Net.Http.Headers.StringWithQualityHeaderValue.Parse("bzip2"));

						if(param != null && param.Headers != null)
						{
							foreach(var pair in param.Headers)
							{
								request.Headers.Add(pair.Key, pair.Value.ToString());
							}
						}

						HttpResponseMessage response;

						if(ct.HasValue)
						{
							response = await client.SendAsync(request, ct.Value);
						}
						else
						{
							response = await client.SendAsync(request);
						}

						Stream s = await response.Content.ReadAsStreamAsync();

						if(!response.IsSuccessStatusCode)
						{
							throw new Exception(String.Format("{0} {1}", (int)response.StatusCode, response.ReasonPhrase)); // reader.ReadToEnd();
						}

						if(response.Content.Headers.ContentEncoding.Any(ce => ce == "bzip2"))
						{
							s = new ICSharpCode.SharpZipLib.BZip2.BZip2InputStream(s);
						}

						DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(Result<T>));

						Result<T> result = (Result<T>)ser.ReadObject(s);

						if(result == null)
						{
							throw new NullReferenceException();
						}

						if(result.Error != null)
						{
							throw new ServiceException(result.Error, result.ErrorCode);
						}

						if((string)method == "InitiateSession")
						{
							_sessionId = result.Obj as string;
						}

						eventArgs._result = result.Obj;

						if(Completed != null)
						{
							Completed(this, eventArgs);
						}

						return result.Obj;
					}
				}
			}
		}

%DEFS%
	}

%DECL%
}
